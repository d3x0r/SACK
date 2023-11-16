// this builds a full library path so working directory can be mis-placed
// however if using packed filesystem need relative path to search
//#define REQUIRE_FULLPATH


// InterShell Modular Interface Layout Konsole
//

// Evo me&nu evolved from Alt Menu
// Evo menu adds supports for resizable/editable buttons, based on a low resolution grid
//	where objects therein occupy a concave set of cells.
//	Objects may not overlap, and when moving, will not move
//	from their prior position until the new position is valid.
//	This then leads us to those spiffy sliding tile puzzles....

//

//#define DEBUG_BACKGROUND_UPDATE
#define LOG_UPDATE_AND_REFRESH_LEVEL LOG_ALWAYS
//#define LOG_UPDATE_AND_REFRESH_LEVEL LOG_NEVER
//#define USE_EDIT_GLARE

#include <stdhdrs.h>
#include <deadstart.h>
#include <filesys.h>
#include <network.h>
#include <idle.h>
#include <pssql.h>
#include <sqlgetoption.h>
#define DECLARE_GLOBAL
#include "intershell_local.h"
#include "resource.h"
#include "loadsave.h"
//#define DEBUG_SCALED_BLOT
#include "widgets/include/banner.h"
#include <psi.h>
#include "pages.h"
#include "intershell_security.h"
#include "intershell_registry.h"
#include "menu_real_button.h"
#include "sprites.h"
#ifndef __NO_ANIMATION__
#include "animation.h"
#include "animation_plugin.h"
#endif
#include "fonts.h"
#ifdef WIN32
#include <vidlib/vidstruc.h>
#endif

INTERSHELL_NAMESPACE
// this structure maintains the current edit button
// what it is, and temp data for configuing it via
// Set/GetCommonButtonControls()



static void (CPROC*EditControlBehaviors)( PSI_CONTROL pc );

enum {
	MENU_CONTROL_BUTTON
	, MENU_CONTROL_CLOCK
};

extern CONTROL_REGISTRATION menu_surface;

// I hate this! but I don't have time to unravel the
// chicken from the egg.
void CPROC AbortConfigureKeys( PSI_CONTROL pc, uint32_t keycode );



struct image_tag{
	CTEXTSTR name;
	Image image;
	int references;
};
static PLIST images;

void InterShell_HideEx( PSI_CONTROL pc_canvas DBG_PASS )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
	_lprintf(DBG_RELAY)( "Hiding Canvas. %p %p", pc_canvas, canvas );
	if( canvas )
		HideDisplay( canvas->renderer );
}

void InterShell_RevealEx( PSI_CONTROL pc_canvas DBG_PASS )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
	if( canvas )
	{
		// check to see if a button is processing still before
		// revealing the surface... might be hiding again (in the case of macro)
		//
		_lprintf(DBG_RELAY)( "Restore Canvas." );
		if( canvas->flags.bButtonProcessing )
		{
			lprintf( "Canvas processing, don't show yet..." );
			canvas->flags.bShowCanvas = 1;
			return;
		}
		RestoreDisplayEx( canvas->renderer DBG_RELAY );
		canvas->flags.bShowCanvas = 0;
	}
}


PSI_CONTROL InterShell_GetCanvas( PPAGE_DATA page )
{
	if( page )
		return page->frame;
	return NULL;
}


Image InterShell_CommonImageLoad( CTEXTSTR name )
{
	struct image_tag *image;
	INDEX idx;
	LIST_FORALL( images, idx, struct image_tag *, image )
	{
		if( strcmp( image->name, name ) == 0 )
		{
			break;
		}
	}
	if( !image )
	{
		image = New( struct image_tag );
		image->name = StrDup( name );
		image->image = LoadImageFileFromGroup( GetFileGroup( "Image Resources", NULL ), image->name );
		image->references = 1;
	}
	else
		image->references++;
	return image->image;
}

void InterShell_CommonImageUnloadByName( CTEXTSTR name )
{
	INDEX idx;
	struct image_tag *image;
	LIST_FORALL( images, idx, struct image_tag *, image )
	{
		if( strcmp( image->name, name ) == 0 )
		{
			break;
		}
	}
	if( image )
	{
		image->references--;
		if( !image->references )
		{
			UnmakeImageFile( image->image );
			Release( (POINTER)image->name );
			Release( image );
			SetLink( &images, idx, NULL );
		}
	}
}

void InterShell_CommonImageUnloadByImage( Image unload )
{
	INDEX idx;
	struct image_tag *image;
	LIST_FORALL( images, idx, struct image_tag *, image )
	{
		if( image->image == unload )
		{
			break;
		}
	}
	if( image )
	{
		image->references--;
		if( !image->references )
		{
			UnmakeImageFile( image->image );
			Release( (POINTER)image->name );
			Release( image );
			SetLink( &images, idx, NULL );
		}
	}
}

//---------------------------------------------------------------------------

static PMENU_BUTTON MouseInControl( PCanvasData canvas, int x, int y )
{
	INDEX idx;
	PMENU_BUTTON pmc;
	PPAGE_DATA current_page;
	current_page = canvas->current_page;
	if( current_page )
	{
		LIST_FORALL( current_page->controls, idx, PMENU_BUTTON, pmc )
		{
			//lprintf( "stuff... %d<=%d %d>%d %d<=%d %d>%d"
			//		 , x, pmc->x
			//		 , x, pmc->x + pmc->w
			//		 , y, pmc->y
			//		 , y, pmc->y + pmc->h );
			if( ( ( (x) >= pmc->x )
				&& ( x < ( pmc->x + pmc->w ) )
				&& ( (y) >= pmc->y )
				&& ( y < ( pmc->y + pmc->h ) )
				) )
			{
				//lprintf( "success - intersect, result FALSE " );
				return pmc;
			}
		}
	}
	return NULL;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

static LOGICAL IsControlSelected( PCanvasData canvas, PMENU_BUTTON pInclude )
{
	INDEX idx;
	PMENU_BUTTON pmc;
	LIST_FORALL( canvas->selected_list, idx, PMENU_BUTTON, pmc )
	{
		if( pmc == pInclude)
			return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------

static int SelectItems( PCanvasData canvas, PMENU_BUTTON pExclude, int x, int y, int w, int h )
{

	INDEX idx;
	int count = 0;
	PMENU_BUTTON pmc;
	PPAGE_DATA current_page;
	current_page = canvas->current_page;
	EmptyList( &canvas->selected_list );
	LIST_FORALL( current_page->controls, idx, PMENU_BUTTON, pmc )
	{
		if( pmc == pExclude )
			continue;
		//lprintf( "stuff... %d<=%d %d>%d %d<=%d %d>%d"
		//		 , x+w, pmc->x
		//		 , x, pmc->x + w
		//		 , y+h, pmc->y
		//		 , y, pmc->y + h );
		if( !( ( ((x)+w)	<= pmc->x )
			|| ( (x)	  >= ( pmc->x + pmc->w ) )
			|| ( ((y)+h) <= pmc->y )
			|| ( (y)	  >= ( pmc->y + pmc->h ) )
			) )
		{
			AddLink( &canvas->selected_list, pmc );
			count++;
		}
	}
	return count;
}


static LOGICAL IsSelectionValidEx( PCanvasData canvas, PMENU_BUTTON pExclude, int x, int y, int *dx, int *dy, int w, int h )
{
	INDEX idx;
	int zero = 0;
	PMENU_BUTTON pmc;
	PPAGE_DATA current_page;
	current_page = canvas->current_page;
	if( !dx )
		dx = &zero;
	if( !dy )
		dy = &zero;
	LIST_FORALL( current_page->controls, idx, PMENU_BUTTON, pmc )
	{
		if( pmc == pExclude )
			continue;
		//lprintf( "stuff... %d<=%d %d>%d %d<=%d %d>%d"
		//		 , x+w, pmc->x
		//		 , x, pmc->x + w
		//		 , y+h, pmc->y
		//		 , y, pmc->y + h );



		if( !( ( ((x+(*dx))+w)	<= pmc->x )
			|| ( (x+(*dx))	  >= ( pmc->x + pmc->w ) )
			|| ( ((y+(*dy))+h) <= pmc->y )
			|| ( (y+(*dy))	  >= ( pmc->y + pmc->h ) )
			) )
		{
			// if the Y delta causes an overlap...
			if( !( ( ((x)+w)	<= pmc->x )
				|| ( (x)	  >= ( pmc->x + pmc->w ) )
				|| ( ((y+(*dy))+h) <= pmc->y )
				|| ( (y+(*dy))	  >= ( pmc->y + pmc->h ) )
				) )
			{
				// maybe if the X delta is okay use it?
				if( !( ( ((x+(*dx))+w)	<= pmc->x )
					|| ( (x+(*dx))	  >= ( pmc->x + pmc->w ) )
					|| ( ((y)+h) <= pmc->y )
					|| ( (y)	  >= ( pmc->y + pmc->h ) )
					) )
				{
					// neither the X or Y delta can be corrected to allivate the issue?
					return FALSE;
				}
				else
				{
					// the x delta alone is okay, clear the Y
					(*dy) = 0;
				}
			}
			else
			{
				// otherwise, the Y delta alone is okay, so clear the X
				(*dx) = 0;
			}

			//lprintf( "Failure - intersect, result FALSE " );
			return TRUE;
		}
	}
	return TRUE;
}

PMENU_BUTTON CreateButton( PSI_CONTROL pc )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );
	PMENU_BUTTON button = New( MENU_BUTTON );
	struct menu_button_colors *colors = &button->base_colors;
	MemSet( button, 0, sizeof( MENU_BUTTON ) );
	button->canvas = canvas;
	// applicaitons end up setting this color...
	//button->page = ShellGetCurrentPage();
	colors->color = BASE_COLOR_BLUE;
	colors->textcolor = BASE_COLOR_WHITE;
	button->glare_set = GetGlareSet( "DEFAULT" );
	SetLink( &button->colors, 0, colors );
	//button->flags.bSquare = TRUE; // good enough default
	return button;
}

LOGICAL InterShell_IsButtonVirtual( PMENU_BUTTON button )
{
	if( button )
	{
		lprintf( "Button is virtual? %d", button->flags.bInvisible );
		return button->flags.bInvisible;
	}
	return 1;
}


void InvokeDestroy( PMENU_BUTTON button )
{
	TEXTCHAR rootname[256];
	void (CPROC*f)(uintptr_t);
	if( button )
	{
		snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
		f = GetRegisteredProcedure2( rootname, void, "button_destroy", (uintptr_t) );
		if( f )
			f(button->psvUser);
	}
}

void InvokeShowControl( PMENU_BUTTON button )
{
	TEXTCHAR rootname[256];
	void (CPROC*f)(uintptr_t);
	snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
	f = GetRegisteredProcedure2( rootname, void, "show_control", (uintptr_t) );
	if( f )
		f(button->psvUser);
}

void InvokeInterShellShutdown( void )
{
	static int bInvoked;
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	if( bInvoked )
		return;
	bInvoked = TRUE;

	InvokeShutdownMacro();

	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/intershell shutdown", &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		void (CPROC *f)(void);
		//snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/common/save common/%s", name );
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (void) );
		if( f )
			f();
	}
	bInvoked = 1;
}

void InvokeBeginEditMode( void )
{
	static int bInvoked;
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	if( bInvoked )
		return;
	bInvoked = 1;

	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/Begin Edit Mode", &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		void (CPROC *f)(void);
		//snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/common/save common/%s", name );
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (void) );
		if( f )
			f();
	}
	bInvoked = 0;
}

void InvokeEndEditMode( void )
{
	static int bInvoked;
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	if( bInvoked )
		return;
	bInvoked = 1;

	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/End Edit Mode", &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		void (CPROC *f)(void);
		//snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/common/save common/%s", name );
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (void) );
		if( f )
			f();
	}
	bInvoked = 0;
}

PCanvasData GetCanvasEx( PSI_CONTROL pc DBG_PASS )
{
	//_lprintf(DBG_RELAY)( "pc is %p", pc );
	{
		ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );
		if( !canvas )
		{
			PSI_CONTROL parent;
			//lprintf( ( "Control %p is not a canvas, go to parent, check it..." ) );
			for( parent = GetParentControl( pc ); parent; parent = GetParentControl( parent ) )
			{
				ValidatedControlData( PCanvasData, menu_surface.TypeID, _canvas, parent );
				if( _canvas )
				{
					canvas = _canvas;
					break;
				}
				//lprintf( ( "Control %p is not a canvas, go to parent, check it..." ) );
			}
		}
		return canvas;
	}
	//return NULL;
}
void InvokeCopyControl( PMENU_BUTTON button )
{
	TEXTCHAR rootname[256];
	void (CPROC*f)(uintptr_t);
	snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
	f = GetRegisteredProcedure2( rootname, void, "copy_control", (uintptr_t) );
	if( f )
		f( button->psvUser );
}

void InvokeCloneControl( PMENU_BUTTON button, PMENU_BUTTON original )
{
	TEXTCHAR rootname[256];
	void (CPROC*f)(uintptr_t,uintptr_t);
	// may validate that button->pTypeName == original->pTypeName
	// due to invokation constraints this would be impossible, until
	// some other developer mangles stuff.
	snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
	f = GetRegisteredProcedure2( rootname, void, "clone_control", (uintptr_t,uintptr_t) );
	if( f )
		f( button->psvUser, original->psvUser );
}

void InvokePasteControl( PMENU_BUTTON button )
{
	TEXTCHAR rootname[256];
	void (CPROC*f)(uintptr_t);
	snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
	f = GetRegisteredProcedure2( rootname, void, "paste_control", (uintptr_t));
	if( f )
		f( button->psvUser );
}

void DestroyButton( PMENU_BUTTON button )
{
	PSI_CONTROL pc_canvas;
	PCanvasData canvas = GetCanvas( pc_canvas = GetParentControl( QueryGetControl( button ) ) );
	if( !canvas )
		return;
	if( button == g.clonebutton )
		InterShell_SetCloneButton( NULL );
	DeleteLink( &canvas->current_page->controls, button );

	InvokeDestroy( button );
	if( button->flags.bListbox )
	{
		DestroyCommon( &button->control.control );
		button->flags.bListbox = 0;
	}
	else if( !button->flags.bCustom )
		DestroyKey( &button->control.key );
	SmudgeCommon( pc_canvas );
}


PSI_CONTROL InterShell_GetButtonCanvas( PMENU_BUTTON button )
{
	button = InterShell_GetPhysicalButton( button );
	if( button && button->canvas )
		return button->canvas->pc_canvas;
	return NULL;
}

PSI_CONTROL CPROC QueryGetControl( PMENU_BUTTON button )
{
	TEXTCHAR rootname[256];
	PSI_CONTROL (CPROC*f)(uintptr_t);
	if( !button )
		return NULL;
	if( button->flags.bListbox )
		return button->control.control;
	else if( button->flags.bCustom )
	{
		snprintf( rootname
			, sizeof( rootname )
			, TASK_PREFIX "/control/%s"
			, button->pTypeName );
		f = GetRegisteredProcedure2( rootname, PSI_CONTROL, "get_control", (uintptr_t) );
		if( f )
			return f(button->psvUser);
	}
	else
		return GetKeyCommon( button->control.key );
	return NULL;
}


uintptr_t CPROC InterShell_GetButtonUserData( PMENU_BUTTON button )
{
	if( !button )
		return 0;
	return button->psvUser;
}


int InvokeFixup( PMENU_BUTTON button )
{
	//if( button->flags.bCustom )
	{
		TEXTCHAR rootname[256];
		void (CPROC*f2)(uintptr_t);
		PMENU_BUTTON prior = configure_key_dispatch.button;
		//PCanvasData prior_canvas = configure_key_dispatch.canvas;
		configure_key_dispatch.button = button;
		snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
		f2 = GetRegisteredProcedure2( rootname, void, "control_fixup", (uintptr_t) );
		if( f2 )
		{
			//lprintf( "running fixup for %s", rootname );
			f2(button->psvUser);
			configure_key_dispatch.button = prior;
			return TRUE;
		}
		configure_key_dispatch.button = prior;
		//else
		//	lprintf( "not running fixup for %s", rootname );
	}
	return FALSE;
}

PGLARE_SET CreateGlareSet( CTEXTSTR name )
{
	PGLARE_SET glare_set = (PGLARE_SET)Allocate( sizeof( *glare_set ) );

	PGLARE_SET check_set;
	INDEX idx;
	TEXTSTR check_name = StrDup( name );
	TEXTSTR theme_check;
	int theme_index = 0;
	if( theme_check = (TEXTSTR)StrChr( check_name, '.' ) )
	{
		theme_check[0] = 0;
		theme_check++;
		theme_index = atoi( theme_check );
	}

	MemSet( glare_set, 0, sizeof( *glare_set ) );

	LIST_FORALL( g.glare_sets, idx, PGLARE_SET, check_set )
	{
		if( StrCaseCmp( check_name, check_set->name ) == 0 )
		{
			glare_set->_theme_set = check_set->_theme_set;
			glare_set->theme_set = check_set->theme_set;
			glare_set->name = StrDup( name );
			SetLink( check_set->theme_set, theme_index, glare_set );
			break;
		}
	}

	if( !check_set )
	{
		glare_set->name = StrDup( check_name );
		AddLink( &g.glare_sets, glare_set );
		glare_set->_theme_set = CreateList();
		glare_set->theme_set = &glare_set->_theme_set;
		SetLink( glare_set->theme_set, 0, glare_set );
	}
	Deallocate( TEXTSTR, check_name );
	return glare_set;
}

PGLARE_SET CheckGlareSet( CTEXTSTR name )
{
	PGLARE_SET glare_set;
	INDEX idx;
	TEXTSTR check_name = StrDup( name );
	TEXTSTR theme_check;
	int theme_index = 0;
	if( theme_check = (TEXTSTR)StrChr( check_name, '.' ) )
	{
		theme_check[0] = 0;
		theme_check++;
		theme_index = atoi( theme_check );
		AddTheme( theme_index );
	}

	LIST_FORALL( g.glare_sets, idx, PGLARE_SET, glare_set )
	{
		if( StrCaseCmp( check_name, glare_set->name ) == 0 )
		{
			if( theme_index )
				return (PGLARE_SET)GetLink( glare_set->theme_set, theme_index );
			break;
		}
	}
	return glare_set;
}
PGLARE_SET GetGlareSet( CTEXTSTR name )
{
	PGLARE_SET glare_set = CheckGlareSet( name );
	if( !glare_set )
		glare_set = CreateGlareSet( name );
	return glare_set;
}

void SetGlareSetFlags( TEXTCHAR const *name, int flags )
{
	PGLARE_SET glare_set = GetGlareSet( name );
	if( glare_set )
	{
		glare_set->flags.bMultiShadeBackground = 0;
		glare_set->flags.bShadeBackground = 0;

		if( flags & GLARE_FLAG_MULTISHADE )
			glare_set->flags.bMultiShadeBackground = 1;
		else if ( flags & GLARE_FLAG_SHADE )
			glare_set->flags.bShadeBackground = 1;
	}
}

void MakeGlareSet( TEXTCHAR const *name, TEXTCHAR const *glare, TEXTCHAR const *up, TEXTCHAR const *down, TEXTCHAR const *mask )
{
	PGLARE_SET glare_set = GetGlareSet( name );
#define SetGlareName(n)	if( glare_set->n ) \
	Release( glare_set->n );				 \
	glare_set->n = n?StrDup( n ):(TEXTCHAR*)NULL;
	SetGlareName( glare );
	SetGlareName( up );
	SetGlareName( down );
	SetGlareName( mask );
}


struct glare_set_edit{
	PGLARE_SET current;
};

void CPROC OnGlareSetSelect( uintptr_t psv, PSI_CONTROL list, PLISTITEM pli )
{
	struct glare_set_edit *params = (struct glare_set_edit*)psv;
	PGLARE_SET glare_set = (PGLARE_SET)GetItemData( pli );
	params->current = glare_set;
	SetControlText( GetNearControl( list, EDIT_GLARESET_GLARE ), glare_set->glare );
	SetControlText( GetNearControl( list, EDIT_GLARESET_UP ), glare_set->up );
	SetControlText( GetNearControl( list, EDIT_GLARESET_DOWN ), glare_set->down );
	SetControlText( GetNearControl( list, EDIT_GLARESET_MASK ), glare_set->mask );
	SetCheckState( GetNearControl( list, CHECKBOX_GLARESET_MULTISHADE )
		, glare_set->flags.bMultiShadeBackground );
	SetCheckState( GetNearControl( list, CHECKBOX_GLARESET_SHADE )
		, glare_set->flags.bShadeBackground );
	SetCheckState( GetNearControl( list, CHECKBOX_GLARESET_FIXED )
		, !glare_set->flags.bMultiShadeBackground &&
		!glare_set->flags.bShadeBackground );
}

void CPROC ApplyGlareSetChanges( uintptr_t psv, PSI_CONTROL button )
{
	struct glare_set_edit *params = (struct glare_set_edit*)psv;
	PGLARE_SET glare_set = params->current;
	TEXTCHAR buffer[256];
#define SetNewGlareImage( gs, var1, var2 )				 \
	if( gs->var1 && StrCaseCmp( buffer, gs->var1 ) )		\
	{															 \
	Release( gs->var1 );					 \
	if( gs->var2 )							\
	{														 \
	UnmakeImageFile( gs->var2 );	 \
	gs->var2 = NULL;					 \
	}	}													 \
	if( buffer[0] )									 \
	gs->var1 = StrDup( buffer );		  \
	else										  \
	gs->var1 = NULL;

	if( !glare_set )return;

	GetControlText( GetNearControl( button, EDIT_GLARESET_GLARE ), buffer, sizeof( buffer ) );
	SetNewGlareImage( glare_set, glare, iGlare );
	GetControlText( GetNearControl( button, EDIT_GLARESET_MASK ), buffer, sizeof( buffer ) );
	SetNewGlareImage( glare_set, mask, iMask );
	GetControlText( GetNearControl( button, EDIT_GLARESET_UP ), buffer, sizeof( buffer ) );
	SetNewGlareImage( glare_set, up, iNormal );
	GetControlText( GetNearControl( button, EDIT_GLARESET_DOWN ), buffer, sizeof( buffer ) );
	SetNewGlareImage( glare_set, down, iPressed );
	glare_set->flags.bMultiShadeBackground = GetCheckState( GetNearControl( button, CHECKBOX_GLARESET_MULTISHADE ) );
	glare_set->flags.bShadeBackground = GetCheckState( GetNearControl( button, CHECKBOX_GLARESET_SHADE ) );
	/* if multi edit, should smudge all pages...*/
	//if( g.current_page )
	//  SmudgeCommon( g.current_page->frame );
}

void CPROC ButtonAddGlareSetTheme( uintptr_t psv, PSI_CONTROL button )
{
	struct glare_set_edit *params = (struct glare_set_edit*)psv;
	int new_number = 0;
	INDEX idx;
	PGLARE_SET theme_set;
	LIST_FORALL( params->current->theme_set[0], idx, PGLARE_SET, theme_set )
		new_number++;
	{
		TEXTCHAR name[256];
		PGLARE_SET newset;
		snprintf( name, 256, "%s.%d", params->current->name, new_number );
		newset = GetGlareSet( name );
		SetItemData( AddListItem( GetNearControl( button, LISTBOX_GLARE_SETS )
											, name )
					  , (uintptr_t)newset );
	}
}


void CPROC ButtonCreateGlareSet( uintptr_t psv, PSI_CONTROL button )
{
	TEXTCHAR buffer[256];
retry:
	if( SimpleUserQuery( buffer, sizeof( buffer ), "Enter New Glareset Name"
		, button ) )
	{
		if( StrStr( buffer, " button " )
			|| StrStr( buffer, " button" )
		  )
		{
			Banner2Message( "Reserved word 'button' used in name." );
			goto retry;
		}
		else if( CheckGlareSet( buffer ) )
		{
			Banner2Message( "Glare set already exists!" );
			goto retry;
		}
		else
		{
			PGLARE_SET glare_set = GetGlareSet( buffer );
			SetItemData( AddListItem( GetNearControl( button, LISTBOX_GLARE_SETS )
				, buffer )
				, (uintptr_t)glare_set );
		}
	}
}

static void glareSetResult( PSI_CONTROL pc, void (*f)( uintptr_t psv, PSI_CONTROL pc, int done, int okay ) ) {
	DestroyFrame( &pc );
	
}

void EditGlareSets( PSI_CONTROL parent )
{
	PSI_CONTROL frame;
	frame = LoadXMLFrame( "EditGlareSets.isFrame" );
	if( frame )
	{
		struct glare_set_edit params;
		int okay = 0;
		int done = 0;
		params.current = NULL;
		SetCommonButtons( frame, &done, &okay );
		{
			PSI_CONTROL list;
			list = GetControl( frame, LISTBOX_GLARE_SETS );
			SetListboxIsTree( list, TRUE );
			if( list )
			{
				PGLARE_SET glare_set;
				INDEX idx;
				LIST_FORALL( g.glare_sets, idx, PGLARE_SET, glare_set )
				{
					INDEX idx2;
					PGLARE_SET theme_set;
					SetItemData( AddListItem( list, glare_set->name ), (uintptr_t)glare_set );
					LIST_FORALL( (*glare_set->theme_set), idx2, PGLARE_SET, theme_set )
					{
						if( idx2 == 0 )
							continue;
						SetItemData( AddListItemEx( list, 1, theme_set->name ), (uintptr_t)theme_set );
					}
				}
				SetSelChangeHandler( list, OnGlareSetSelect, (uintptr_t)&params );
			}
			SetButtonGroupID( GetControl( frame, CHECKBOX_GLARESET_MULTISHADE ), 2 );
			SetButtonGroupID( GetControl( frame, CHECKBOX_GLARESET_SHADE ), 2 );
			SetButtonGroupID( GetControl( frame, CHECKBOX_GLARESET_FIXED ), 2 );
			SetButtonPushMethod( GetControl( frame, GLARESET_APPLY_CHANGES ), ApplyGlareSetChanges, (uintptr_t)&params );
			SetButtonPushMethod( GetControl( frame, GLARESET_CREATE ), ButtonCreateGlareSet, (uintptr_t)&params );
			SetButtonPushMethod( GetControl( frame, GLARESET_ADD_THEME ), ButtonAddGlareSetTheme, (uintptr_t)&params );
		}
		DisplayFrameOver( frame, parent );
		PSI_HandleStatusEvent( frame, glareSetResult, 0 );
	}
}


//---------------------------------------------------------------------------


CTEXTSTR InterShell_GetSystemName( void )
{
	return g.system_name;
}


void InterShell_SetButtonStyle( PMENU_BUTTON button, TEXTCHAR const *style )
{
	if( !button )
		button = configure_key_dispatch.button;
	if( button )
		button->glare_set = GetGlareSet( style );
}

//---------------------------------------------------------------------------

static void SetButtonText( PMENU_BUTTON button )
{
	if( !button )
		button = configure_key_dispatch.button;
	if( button )
	{
		if( button->flags.bListbox )
			return;
		if( button->flags.bCustom )
		{
			//lprintf( "Custom control send a smudge..." );
			SmudgeCommon( QueryGetControl( button ) );
			return;
		}
		{
			TEXTCHAR button_text[256];
			// take data in psv and apply to the button appropriately...
			CTEXTSTR p = InterShell_TranslateLabelTextEx( button, NULL, button_text, sizeof( button_text ), button->text );
			TEXTCHAR newmenuname[256];
			int pos;
			// Get info from dialog...
			newmenuname[0] = 'A';
			pos = 1;
			if( p )
			{
				while( p && p[0] )
				{
					if( p[0] == '_' )
					{
						newmenuname[pos++] = '\0';
						newmenuname[pos++] = 'A';
					}
					else
					{
						newmenuname[pos++] = p[0];
					}
					p++;
				}
				// double null terminate string.
				newmenuname[pos++] = 0;
				newmenuname[pos++] = 0;
				//p = Allocate( pos );
				if( button->control.key )
					SetKeyText( button->control.key, newmenuname );
			}
			else
			{
				if( button->control.key )
					SetKeyText( button->control.key, NULL );
			}
		}
	}
}

//---------------------------------------------------------------------------

void FlushToKey( PMENU_BUTTON button )
{
				if( button->control.key )
				{
					struct menu_button_colors *colors = (struct menu_button_colors *)GetLink( &button->colors, g.theme_index );
					if( !colors )
						colors = (struct menu_button_colors *)GetLink( &button->colors, 0 );
					EnableKeyPresses( button->control.key, !button->flags.bNoPress );
					SetKeyTextColor( button->control.key, colors->textcolor );
					SetKeyColor( button->control.key, colors->color );
					SetButtonText( button );
					if( button->glare_set->flags.bMultiShadeBackground )
					{
						if( colors->secondary_color )
						{
							SetKeyMultiShading( button->control.key, 0 //BASE_COLOR_BLACK //BASE_COLOR_DARKGRAY
													, colors->color
													, colors->secondary_color
													);
							SetKeyMultiShadingHighlights( button->control.key, 0
																 , colors->color
																 , colors->highlight_color );
						}
						else
						{
							SetKeyMultiShading( button->control.key
													, 0 //button->color
													, colors->color
													, colors->color
													);
						}
					}
					else if( button->glare_set->flags.bShadeBackground )
						SetKeyShading( button->control.key, colors->color );
					else
						SetKeyShading( button->control.key, 0 ); // magic value to remove shading factor.
					if( button->pImage[0] )
					{
						if( !button->decal_image )
							button->decal_image = InterShell_CommonImageLoad( button->pImage );
						SetKeyImage( button->control.key, button->decal_image );
						SetKeyImageMargin( button->control.key, button->decal_horiz_margin, button->decal_vert_margin );
						SetKeyImageAlpha( button->control.key, button->decal_alpha );
					}
					else
					{
						if( button->decal_image )
						{
							InterShell_CommonImageUnloadByImage( button->decal_image );
							button->decal_image = NULL;
						}
						SetKeyImage( button->control.key, NULL );
						SetKeyImageAlpha( button->control.key, 0 );
					}
#ifndef __NO_ANIMATION__
					if( button->pAnimation[0] )
					{
						PCanvasData canvas = GetCanvas( GetParentControl( QueryGetControl( button ) ) );
						if( button->decal_animation )
							DeInitAnimationEngine( button->decal_animation );

						button->decal_animation = InitAnimationEngine();
						GenerateAnimation( button->decal_animation, g.single_frame /*pc_button*/ /*GetFrameRenderer( g.single_frame)*/
											  , button->pAnimation, PARTX(button->x), PARTY(button->y), PARTW( button->x, button->w ), PARTH( button->y, button->h ));

					}
					else
					{
						if( button->decal_animation )
						{
							DeInitAnimationEngine( button->decal_animation );
							button->decal_animation = NULL;
						}
					}
#endif
					//SetKeyColor( button->key, button->color );
				}
}

void FixupButtonEx( PMENU_BUTTON button DBG_PASS )
{
	int show;
	PSI_CONTROL pc_button = QueryGetControl( button );
	PCanvasData canvas = button->canvas;
	//lprintf( "Button fixup..." );
//#define LoadImg(n) ((n)?LoadImageFileFromGroup(GetFileGroup( "Image Resources", NULL ), (TEXTSTR)n):NULL)
#define LoadImg(n) ((n)?LoadImageFile((TEXTSTR)n):NULL)
	if( canvas )
		if( canvas->flags.bEditMode ) // don't do fixup/reveal if editing...
		{
#ifndef __NO_ANIMATION__
			if( button->decal_animation )
			{
				DeInitAnimationEngine( button->decal_animation );
				button->decal_animation = NULL;
			}
#endif
			return;
		}
		//lprintf( "--- updating a button's visual aspects, disable updates..." );
		//	AddUse( pc_button );
		if( !g.flags.bPageUpdateDisabled )
			EnableCommonUpdates( pc_button, FALSE );
		show = QueryShowControl( button );
		if( !show )
		{
			/* this doesn't matter... */
			HideControl( pc_button );
#ifndef __NO_ANIMATION__
			if( button->decal_animation )
			{
				DeInitAnimationEngine( button->decal_animation );
				button->decal_animation = NULL;
			}
#endif
			return;
		}
		if( !button->flags.bCustom &&
			!button->flags.bListbox )
		{
			PGLARE_SET glare_set = button->glare_set;
			if( glare_set )
			{
				//TEXTCHAR buf[2566];
				//GetCurrentPath( buf, sizeof(buf) );
				//lprintf( "Current path %s", buf );
				// good a time as any to load images for the glare set...
				if( !glare_set->iGlare && glare_set->glare )
				{
					glare_set->iGlare = LoadImg( glare_set->glare );
				}
				if( !glare_set->iPressed && glare_set->down )
				{
					glare_set->iPressed = LoadImg( glare_set->down );
				}
				if( !glare_set->iNormal && glare_set->up )
				{
					glare_set->iNormal = LoadImg( glare_set->up );
				}
				if( !glare_set->iMask && glare_set->mask )
				{
					glare_set->iMask = LoadImg( glare_set->mask );
				}
				if( button->control.key )
				{
					SetKeyLenses( button->control.key, glare_set->iGlare, glare_set->iPressed, glare_set->iNormal, glare_set->iMask );
				}
			}
			else
			{
				if( button->control.key )
					SetKeyLenses( button->control.key, NULL, NULL, NULL, NULL );
			}
		}
		//_xlprintf(LOG_NOISE DBG_RELAY)( "Fixing up button :%s", button->pTypeName );
		{
			// fixup can set ... background color?
			// text color?
			if( button->font_preset )
				SetCommonFont( pc_button, (*button->font_preset) );
			InvokeFixup( button ); // no fixup method...
			if( !button->flags.bCustom && !button->flags.bListbox && button->control.key )
			{
				// not a custom button, therefore fixup common properties of a button control...
				FlushToKey( button );
			}
			// allow user method to override anything pre-set in common
		}
		if( show )
		{
			//lprintf( "-!!!!! ---- reveal?" );
			RevealCommon( pc_button );
		}

		if( !g.flags.bPageUpdateDisabled )
		{
			//lprintf( "allow button to smudge... " );
			EnableCommonUpdates( pc_button, TRUE );
			SmudgeCommon( pc_button );
		}
		//	DeleteUse( pc_button );
		//lprintf( "--- updated a button's visual aspects, disable updates..." );
}

void CPROC InterShell_DisablePageUpdate( PSI_CONTROL pc_canvas, LOGICAL bDisable )
{
	INDEX idx;
	INDEX idx_page;
	PCanvasData canvas = GetCanvas( pc_canvas );
	PPAGE_DATA current_page;
	PMENU_BUTTON control;
	//lprintf( "------- %s Page Updates ----------", bDisable?"DISABLE":"ENABLE" );
	g.flags.bPageUpdateDisabled = bDisable;

	//IJ 05.15.2007

	if( !canvas )
		return;


	if( g.flags.multi_edit )
	{
		//IJ 05.15.2007		if( !canvas )
		//IJ							 return;

		LIST_FORALL( canvas->pages, idx_page, PPAGE_DATA, current_page )
		{
			LIST_FORALL( current_page->controls, idx, PMENU_BUTTON, control )
			{
				EnableCommonUpdates( QueryGetControl( control ), !bDisable );
			}
			EnableCommonUpdates( current_page->frame, !bDisable );
			if( !bDisable )
				SmudgeCommon( current_page->frame );
		}
		{
			LIST_FORALL( canvas->default_page->controls, idx, PMENU_BUTTON, control )
			{
				EnableCommonUpdates( QueryGetControl( control ), !bDisable );
			}
			EnableCommonUpdates( canvas->default_page->frame, !bDisable );
			if( !bDisable )
				SmudgeCommon( canvas->default_page->frame );
		}
	}
	else
	{
 		//lprintf( "disable update..." );
		if( ( current_page = ShellGetCurrentPage( pc_canvas ) ) )
		{
			LIST_FORALL( current_page->controls, idx, PMENU_BUTTON, control )
			{
				EnableCommonUpdates( QueryGetControl( control ), !bDisable );
			}
			EnableCommonUpdates( current_page->frame, !bDisable );
			if( !bDisable )
			{
				SmudgeCommon( current_page->frame );
			}
		}
	}
}


int InvokeEditEnd( PMENU_BUTTON button )
{
	TEXTCHAR rootname[256];
	void (CPROC*f)(uintptr_t);
	snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
	f = GetRegisteredProcedure2( rootname, void, "on_menu_edit_end", (uintptr_t) );
	if( f )
	{
		f(button->psvUser);
		return TRUE;
	}
	return FALSE;
}

int QueryShowControl( PMENU_BUTTON button )
{
	TEXTCHAR rootname[256];
	LOGICAL (CPROC*f)(uintptr_t);

	if( !g.flags.multi_edit && button->canvas )
		if( button->page != button->canvas->current_page )
			return 0;
	if( button->show_on )
	{
		INDEX idx;
		PTEXT name;
		int checked = 0;
		LIST_FORALL( button->show_on, idx, PTEXT, name )
		{
			checked++;
			if( CompareMask( GetText( name ), g.system_name, FALSE ) )
				break;
		}
		if( !name && checked )
			return 0;
	}
	if( button->no_show_on )
	{
		INDEX idx;
		PTEXT name;
		LIST_FORALL( button->no_show_on, idx, PTEXT, name )
		{
			if( CompareMask( GetText( name ), g.system_name, FALSE ) )
				break;
		}
		if( name )
			return 0;
	}
	snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
	f = GetRegisteredProcedure2( rootname, LOGICAL, "query can show", (uintptr_t) );
	if( f )
	{
		return f(button->psvUser);
	}
	// if they control does not support this, show it.
	return TRUE;
}

void InvokeEditBegin( PMENU_BUTTON button )
{
	TEXTCHAR rootname[256];
	void (CPROC*f)(uintptr_t);
	snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
	f = GetRegisteredProcedure2( rootname, void, "on_menu_edit_begin", (uintptr_t) );
	if( f )
		f(button->psvUser);
}

static void CPROC AllPresses( uintptr_t psv, PKEY_BUTTON key )
{
	PMENU_BUTTON button = (PMENU_BUTTON)psv;
	PCanvasData canvas = button?button->canvas:NULL;
	uintptr_t psv_security;

	// serialize button presses to allow one to complete (also a quick hack for handling
	// password pause dialogs that do not cover screen... )
	if( canvas->flags.bDoingButton )
		return;
	canvas->flags.bButtonProcessing = 1;
	canvas->flags.bDoingButton = 1;
	// if the security context fails, then abort everything about the button.
	if( ( psv_security = CreateSecurityContext( button->psvUser ) ) == INVALID_INDEX )
	{
		//if( button->iSecurityContext == INVALID_INDEX )
		canvas->flags.bButtonProcessing = 0;
		if( canvas->flags.bShowCanvas )
			InterShell_Reveal( canvas->pc_canvas );
		canvas->flags.bDoingButton = 0;
		return;
	}

	GenerateSprites( GetFrameRenderer( canvas->pc_canvas ), PARTX(button->x) + (PARTW(button->x,button->w) /2), PARTY(button->y) + (PARTH(button->y,button->h)/2));

	if( g.flags.bLogKeypresses )
		lprintf( "Issue keypress : [%s]%s", button->text, button->pTypeName );

	if( button->original_keypress )
		button->original_keypress( button->psvUser );
	//lprintf( "Restore should have happened..." );
	if( button->pPageName )
	{
		if( canvas && !button->flags.bIgnorePageChange )
		{
			//lprintf( "Changing pages, but only virtually don't activate the page always" );
			ShellSetCurrentPage( canvas->pc_canvas, button->pPageName );
		}
		button->flags.bIgnorePageChange = 0;
	}

	/* If there is a handle, call close security on it; -1 would have failed above, 0 is no security on button... */
	if( psv_security )
		CloseSecurityContext( (uintptr_t)button, psv_security );

	// stop doing button, allow other presses to happen...
	canvas->flags.bDoingButton = 0;
	canvas->flags.bButtonProcessing = 0;
	if( canvas->flags.bShowCanvas )
		InterShell_Reveal( canvas->pc_canvas );
	if( g.flags.bLogKeypresses )
		lprintf( "Finish keypress : [%s]%s", button->text, button->pTypeName );

}

static void CPROC ListBoxSelectionChanged( uintptr_t psv, PSI_CONTROL list, PLISTITEM pli )
{
	PMENU_BUTTON button = (PMENU_BUTTON)psv;
	{
		TEXTCHAR rootname[256];
		void (CPROC*f)(uintptr_t,PLISTITEM);
		snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
		f = GetRegisteredProcedure2( rootname, void, "listbox_selection_changed", (uintptr_t,PLISTITEM) );
		if( f )
			f(button->psvUser, pli);
		{
			CTEXTSTR name;
			PCLASSROOT data = NULL;
			snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s/listbox_selection_changed", button->pTypeName );
			for( name = GetFirstRegisteredName( rootname, &data );
				name;
				name = GetNextRegisteredName( &data ) )
			{
				f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (uintptr_t,PLISTITEM) );
				if( f )
					f(button->psvUser,pli);
			}
		}
	}

}

static void CPROC ListBoxDoubleChanged( uintptr_t psv, PSI_CONTROL list, PLISTITEM pli )
{
	PMENU_BUTTON button = (PMENU_BUTTON)psv;
	{
		TEXTCHAR rootname[256];
		void (CPROC*f)(uintptr_t,PLISTITEM);
		snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
		f = GetRegisteredProcedure2( rootname, void, "listbox_double_changed", (uintptr_t,PLISTITEM) );
		if( f )
			f(button->psvUser, pli);
		{
			CTEXTSTR name;
			PCLASSROOT data = NULL;
			snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s/listbox_double_changed", button->pTypeName );
			for( name = GetFirstRegisteredName( rootname, &data );
				name;
				name = GetNextRegisteredName( &data ) )
			{
				f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (uintptr_t,PLISTITEM) );
				if( f )
					f(button->psvUser,pli);
			}
		}
	}

}

// return FALSE creation method fails.
static LOGICAL InvokeButtonCreate( PSI_CONTROL pc_canvas, PMENU_BUTTON button, LOGICAL bVisible )
{
	TEXTCHAR rootname[256];
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
	uintptr_t (CPROC*f)(PSI_CONTROL,int32_t x, int32_t y, uint32_t w, uint32_t h);
	snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
	button->flags.bNoCreateMethod = TRUE; // assume there's no creator for this control
	lprintf( "Invoke button create %s", button->pTypeName );
	g.CurrentlyCreatingButtonType = button->pTypeName;
	g.CurrentlyCreatingButton = button;
	//lprintf( "..." );
	//if( StrCmp( button->pTypeName, "Task" ) == 0 )
	//	DebugBreak();
	if( canvas )
		f = GetRegisteredProcedure2( rootname, uintptr_t, "control_create", (PSI_CONTROL,int32_t,int32_t,uint32_t,uint32_t) );
	else
		f = NULL;
	if( f )
	{
		button->flags.bNoCreateMethod = 0; // found a creation method for this button.
		button->flags.bCustom = TRUE;
		button->psvUser = f( canvas->current_page->frame
			, PARTX( button->x )
			, PARTY( button->y )
			, PARTW( button->x, button->w )
			, PARTH( button->y, button->h )
								 );
		if( button->psvUser == 1 )
		{
			lprintf( "Button type '%s' returned 1 as a result.", button->pTypeName );
		}
		HideControl( QueryGetControl( button ) );

	}
	else
	{
		if( bVisible )
		{
			uintptr_t (CPROC*f_list)(PSI_CONTROL);
			if( canvas )
				f_list = GetRegisteredProcedure2( rootname, uintptr_t, "listbox_create", (PSI_CONTROL) );
			else
				f_list = NULL;
			if( f_list )
			{
				button->flags.bNoCreateMethod = 0; // found a creation method for this button.
				button->flags.bListbox = TRUE;
				button->control.control = MakeNamedControl( canvas->current_page->frame, LISTBOX_CONTROL_NAME
					, PARTX( button->x )
					, PARTY( button->y )
					, PARTW( button->x, button->w )
					, PARTH( button->y, button->h )
					, -1
					);
				SetSelChangeHandler( button->control.control, ListBoxSelectionChanged, (uintptr_t)button );
				SetDoubleClickHandler( button->control.control, ListBoxDoubleChanged, (uintptr_t)button );
				SetListboxSort( button->control.control, LISTBOX_SORT_DISABLE );
				button->psvUser = f_list( button->control.control );
				if( !button->psvUser )
				{
					lprintf( "User Create failed (return psv=0), hiding control." );
					HideControl( button->control.control );
					DestroyCommon( &button->control.control );
					button->flags.bListbox = 0;
				}
			}
			else
			{
				button->psvUser = 0;
				button->control.control = NULL;
			}
			// allow user to override default selection methods...
			// maybe?
		}
		if( ( !bVisible || canvas ) && !button->psvUser )
		{
			if( bVisible )
			{
				button->control.key = MakeKeyExx( canvas->current_page->frame
				, PARTX( button->x )
				, PARTY( button->y )
				, PARTW( button->x, button->w )
				, PARTH( button->y, button->h )
				, 0
				, NULL//g.iGlare
				, NULL//g.iNormal
				, NULL//g.iPressed
				, NULL//g.iMask
				, 0 // set to image and pass image pointer for texture
				, BASE_COLOR_BLACK
				, "AUndefined\0"
				, (*button->font_preset)
				, NULL // user click function
				, button->pTypeName  // user click function name (default)
				, 0  // this gets set later...
				, 0 );
				HideKey( button->control.key );
			}
			else
			{
				{
					SimplePressHandler handler;
					TEXTCHAR realname[256];
					snprintf( realname, sizeof(realname), "sack/widgets/keypad/press handler/%s", button->pTypeName );
#define GetRegisteredProcedure2(nc,rtype,name,args) (rtype (CPROC*)args)GetRegisteredProcedureEx((nc),#rtype, name, #args )

					handler = GetRegisteredProcedure2( realname, void, "on_keypress_event", (uintptr_t) );
					if( handler )
					{
						button->flags.bNoCreateMethod = 0; // found a creation method for this button.
						button->original_keypress = handler;
					}
				}
			}
			{
				uintptr_t (CPROC*f)(PMENU_BUTTON button);
				f = GetRegisteredProcedure2( rootname, uintptr_t, "button_create", (PMENU_BUTTON) );
				if( f )
				{
					button->flags.bNoCreateMethod = 0; // found a creation method for this button.
					button->psvUser = f(button);
				}
				if( button->control.key )
				{
					// hide the key anyway; we're always creating in a hidden type mode...
					HideKey( button->control.key );
					if( !button->psvUser )
					{
					}
					else
					{
						// sneak in and nab the keypress event.. forward it later...
						GetKeySimplePressEvent( button->control.key, &button->original_keypress, NULL );
						SetKeyPressEvent( button->control.key, AllPresses, (uintptr_t)button );
					}
				}
			}
		}
	}
	g.CurrentlyCreatingButtonType = NULL;
	g.CurrentlyCreatingButton = NULL;
	//lprintf( "..." );
	lprintf( "cleared button create" );
	return TRUE;
}

PMENU_BUTTON CreateInvisibleControl( PSI_CONTROL pc_frame, TEXTCHAR *name )
{
	if( name )
	{
		PMENU_BUTTON button = CreateButton( pc_frame );
		button->flags.bCustom = FALSE;
		button->flags.bListbox = FALSE;
		button->flags.bInvisible = TRUE;
		button->pTypeName = StrDup( name );
		button->font_preset = NULL;//UseACanvasFont( pc_frame, "Default" );
		button->font_preset_name = NULL;//StrDup( "Default" );
		//lprintf( "Creating a virtual control %s", name );
		InvokeButtonCreate( NULL, button, FALSE );
		return button;
	}
	return NULL;
}

uintptr_t InterShell_GetButtonExtension( PMENU_BUTTON button )
{
	if( button )
		return button->psvUser;
	return 0;
}

PMENU_BUTTON CPROC CreateSomeControl( PSI_CONTROL pc_canvas, int x, int y, int w, int h
								, CTEXTSTR name )
{
	PMENU_BUTTON button = CreateButton( pc_canvas );
	PMENU_BUTTON prior = configure_key_dispatch.button;
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
	configure_key_dispatch.button = button;
	button->flags.bCustom = FALSE;
	button->flags.bListbox = FALSE;
	button->x = x;
	button->y = y;
	button->w = w;
	button->h = h;
	button->pTypeName = StrDup( name );
	button->font_preset = UseACanvasFont( pc_canvas, "Default" );
	button->font_preset_name = StrDup( "Default" );

	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, SetAlpha( BASE_COLOR_NICE_ORANGE, 150 ), SetAlpha( BASE_COLOR_BLACK, 128 ), SetAlpha( BASE_COLOR_LIGHTGREEN, 192 ) );
	if( !canvas->current_page->canvas )
		canvas->current_page->canvas = canvas;
	PutButtonOnPage( canvas->current_page, button );
	InvokeButtonCreate( pc_canvas, button, TRUE );

	configure_key_dispatch.button = prior;
	return button;
}

uintptr_t  InterShell_CreateControl( PSI_CONTROL canvas, CTEXTSTR type, int x, int y, int w, int h )
{
	PMENU_BUTTON button = CreateSomeControl( canvas, x, y, w, h, type );
	return button->psvUser;
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

CTEXTSTR CPROC InterShell_GetButtonFontName( PMENU_BUTTON pc )
{
	if( pc )
		return pc->font_preset_name;
	return NULL;
}

SFTFont* CPROC InterShell_GetButtonFont( PMENU_BUTTON pc )
{
	if( pc )
		return pc->font_preset;
	return NULL;
}


SFTFont* CPROC InterShell_GetCurrentButtonFont( void )
{
	return InterShell_GetButtonFont( configure_key_dispatch.button );
}


PMENU_BUTTON CPROC InterShell_GetCurrentButton( void )
{
	return ( configure_key_dispatch.button );
}

PMENU_BUTTON InterShell_GetPhysicalButton( PMENU_BUTTON button )
{
	//lprintf( "Finding container of %p %p", button, button?button->container_button:NULL );
	while( button && button->container_button )
	{
		button = button->container_button;
		//lprintf( "Finding container of %p %p", button, button->container_button );
	}
	return button;
}

void InterShell_SetButtonFont( PMENU_BUTTON button, SFTFont *font )
{
	if( button && font )
		button->font_preset = font;
}

void CPROC InterShell_SetButtonFontName( PMENU_BUTTON button, CTEXTSTR name )
{
	if( button )
	{
		if( ( button->font_preset_name && name && StrCaseCmp( button->font_preset_name, name ) )
			|| ( button->font_preset_name && !name )
			|| ( name && !button->font_preset_name ) )
		{
			if( button->font_preset_name )
				Release( (POINTER)button->font_preset_name );
			button->font_preset_name = StrDup( name );
			button->font_preset = UseACanvasFont( button->canvas->pc_canvas, button->font_preset_name );
		}
	}
}

//---------------------------------------------------------------------------

void InterShell_GetButtonText( PMENU_BUTTON button, TEXTSTR text, int text_buf_len )
{
	if( text )
	{
		if( button && button->text)
			StrCpyEx( text, button->text, text_buf_len );
		else
			text[0] = 0;
	}
}


void InterShell_SetButtonText( PMENU_BUTTON button, CTEXTSTR text )
{
	//lprintf( "Want to set button %p to %s", button, text );
	if( button )
	{
		if( button->text )
			Release( button->text );
		button->text = StrDup( text );
		SetButtonText( button );
	}
}

//---------------------------------------------------------------------------

void InterShell_SetButtonImage( PMENU_BUTTON button, CTEXTSTR name )
{
	if( button )
	{
		// already have this image on the button?
		if( name && ( StrCaseCmp( name, button->pImage ) == 0 ) )
			return;
		// it's not the same image, so we need to unload...
		if( button->decal_image )
		{
			InterShell_CommonImageUnloadByImage( button->decal_image );
			button->decal_image = NULL;
		}
		// set the new name into name string...
		if( name )
			StrCpyEx( button->pImage, name, sizeof( button->pImage ) );
		else
			button->pImage[0] = 0;
	}
}

//---------------------------------------------------------------------------

#ifndef __NO_ANIMATION__
INTERSHELL_PROC( void, InterShell_SetButtonAnimation )( PMENU_BUTTON button, CTEXTSTR name )
{
	if( button )
	{
		// already have this animation on the button?
		if( name && ( StrCaseCmp( name, button->pAnimation ) == 0 ) )
			return;
		// it's not the same animation, so we need to unload...
		if( button->decal_animation )
		{
			DeInitAnimationEngine( button->decal_animation );
			button->decal_animation = NULL;
		}
		// set the new name into name string...
		if( name )
			strcpy( button->pAnimation, name );
		else
			button->pAnimation[0] = 0;
	}
}
#endif
//---------------------------------------------------------------------------

INTERSHELL_PROC( void, InterShell_SetButtonImageAlpha )( PMENU_BUTTON button, int16_t alpha )
{
	if( button )
	{
		// already have this image on the button?
		button->decal_alpha = alpha;
	}
}

//---------------------------------------------------------------------------

INTERSHELL_PROC( void, InterShell_SetButtonColors )( PMENU_BUTTON button, CDATA cText, CDATA cBackground1, CDATA cBackground2, CDATA cBackground3 )
{
	struct menu_button_colors *colors;

	if( !button )
		button = configure_key_dispatch.button;
	if( !button )
		return;

	colors = (struct menu_button_colors*)GetLink( &button->colors, g.theme_index );
	if( !colors )
		colors = (struct menu_button_colors*)GetLink( &button->colors, 0 );


	button = InterShell_GetPhysicalButton( button );
	if( button && button->glare_set && button->glare_set->flags.bMultiShadeBackground )
	{
		if( cBackground1 )
		{
			if( cBackground1 == COLOR_DISABLE )
				colors->color = 0;
			else
				colors->color = cBackground1;
		}
		if( cBackground2 )
		{
			if( cBackground2 == COLOR_DISABLE )
				colors->secondary_color = 0;
			else
				colors->secondary_color = cBackground2;
		}
		if( cBackground3 )
		{
			if( cBackground3 == COLOR_DISABLE )
				colors->highlight_color = 0;
			else
				colors->highlight_color = cBackground3;
		}
		if( cText )
		{
			if( cText == COLOR_DISABLE )
				colors->textcolor = 0;
			else
				colors->textcolor = cText;
		}
	}
	else if( button )
	{
		if( cText )
		{
			if( cText == COLOR_DISABLE )
				colors->textcolor = 0;
			else
				colors->textcolor = cText;
		}
		if( cBackground1 )
		{
			if( cBackground1 == COLOR_DISABLE )
				colors->color = 0;
			else
				colors->color = cBackground1;
		}
	}
}


//---------------------------------------------------------------------------

INTERSHELL_PROC( void, InterShell_SetButtonColor )( PMENU_BUTTON button, CDATA primary, CDATA secondary )
{
	InterShell_SetButtonColors( button, 0, primary, secondary, 0 );
}

//---------------------------------------------------------------------------

INTERSHELL_PROC( void, InterShell_GetButtonColors )( PMENU_BUTTON button, CDATA *primary, CDATA *secondary
										, CDATA *ring_color
										, CDATA *highlight_ring_color )
{
	if( !button )
		button = configure_key_dispatch.button;
	button = InterShell_GetPhysicalButton( button );
	if( button )
	{
		struct menu_button_colors *colors = (struct menu_button_colors*)GetLink( &button->colors, g.theme_index );
		if( !colors )
			colors = (struct menu_button_colors*)GetLink( &button->colors, 0 );
		if( primary )
			(*primary) = colors->textcolor;
		if( secondary )
			(*secondary) = colors->color;
		if( ring_color )
			(*ring_color ) = colors->secondary_color;
		if( highlight_ring_color )
			(*highlight_ring_color ) = colors->highlight_color;
	}
}

//---------------------------------------------------------------------------

void AddSystemName( PSI_CONTROL list, CTEXTSTR name )
{
	struct system_info *system = g.systems;
	for( ;system; system = NextLink( system ) )
	{
		if( TextLike( system->name, name ) )
			break;
	}
	if( !system )
	{
		system = New( struct system_info );
		MemSet( system, 0, sizeof( struct system_info ) );
		system->name = SegCreateFromText( name );
		LinkThing( g.systems, system );
		if( list )
		{
			AddListItem( list, name );
		}
	}
}

//---------------------------------------------------------------------------

void GetAllowDisallowControls( void )
{
	PSI_CONTROL frame = configure_key_dispatch.frame;
	PMENU_BUTTON button = configure_key_dispatch.button;
	if( !button || button->flags.bInvisible )
		return;
	{
		PSI_CONTROL list;
		INDEX idx;
		PLISTITEM pli;
		list = GetControl( frame, LIST_SYSTEMS );
		if( list )
		{
			for( idx = 0; ( pli = GetNthItem( list, (int)idx ) ); idx++ )
			{
				TEXTCHAR buffer[256];
				GetListItemText( pli, buffer, sizeof( buffer ) );
				{
					AddSystemName( NULL, buffer );
				}
			}
		}
		list = GetControl( frame, LIST_ALLOW_SHOW );
		if( list )
		{
			PTEXT name;
			LIST_FORALL( button->show_on, idx, PTEXT, name )
			{
				LineRelease( name );
				SetLink( &button->show_on, idx, NULL );
			}
			for( idx = 0; ( pli = GetNthItem( list, (int)idx ) ); idx++ )
			{
				TEXTCHAR buffer[256];
				GetListItemText( pli, buffer, sizeof( buffer ) );
				{
					INDEX idx;
					LIST_FORALL( button->show_on, idx, PTEXT, name )
					{
						if( TextLike( name, buffer ) )
							break;
					}
					if( !name )
						AddLink( &button->show_on, SegCreateFromText( buffer ) );
				}
			}
		}
		list = GetControl( frame, LIST_DISALLOW_SHOW );
		if( list )
		{
			PTEXT name;
			LIST_FORALL( button->no_show_on, idx, PTEXT, name )
			{
				LineRelease( name );
				SetLink( &button->no_show_on, idx, NULL );
			}
			for( idx = 0; ( pli = GetNthItem( list, (int)idx ) ); idx++ )
			{
				TEXTCHAR buffer[256];
				GetListItemText( pli, buffer, sizeof( buffer ) );
				{
					INDEX idx;
					LIST_FORALL( button->no_show_on, idx, PTEXT, name )
					{
						if( TextLike( name, buffer ) )
							break;
					}
					if( !name )
						AddLink( &button->no_show_on, SegCreateFromText( buffer ) );
				}
			}
		}
	}
}

//---------------------------------------------------------------------------

void GetCommonButtonControls( PSI_CONTROL frame )
{
	PSI_CONTROL pc;
	// test for same frame?
	// this should have been set already by SetCommonButtonControls.
	if( configure_key_dispatch.frame != frame )
	{
		lprintf( "Aren't we busted?  isn't there more than one config dialog up?!" );
		DebugBreak();
	}
	if( !configure_key_dispatch.button ) // nothing for this to do... nothing ocmmon about it.
		return;

	GetAllowDisallowControls();
	// custom controls don't have a button characteristic like this...
	configure_key_dispatch.button->flags.bConfigured = 1;
	{
		struct menu_button_colors *colors = (struct menu_button_colors*)GetLink( &configure_key_dispatch.button->colors, g.theme_index );
		if( !colors )
			colors = (struct menu_button_colors*)GetLink( &configure_key_dispatch.button->colors, 0 );
		colors->color = GetColorFromWell( GetControl( frame, CLR_BACKGROUND ) );
		colors->secondary_color = GetColorFromWell( GetControl( frame, CLR_RING_BACKGROUND ) );
		colors->textcolor = GetColorFromWell( GetControl( frame, CLR_TEXT_COLOR ) );
		colors->highlight_color = GetColorFromWell( GetControl( frame, CLR_RING_HIGHLIGHT ) );
	}
	{
		TEXTCHAR buffer[128];
		PSI_CONTROL list = GetControl( frame, LST_PAGES );
		buffer[0] = 0; // this should have been cleard by GetItemText, but it's lazy, apparently
		if( list )
		{
			PLISTITEM pli = GetSelectedItem( list );
			GetItemText( pli, sizeof( buffer ), buffer );
			if( StrCaseCmp( buffer, "-- NONE --" ) == 0 )
				buffer[0] = 0;
			else if( StrCaseCmp( buffer, "-- Startup Page" ) == 0 )
				StrCpyEx( buffer, "first", sizeof( buffer ) );
			else if( StrCaseCmp( buffer, "-- Next" ) == 0 )
				StrCpyEx( buffer, "next", sizeof( buffer ) );
			else if( StrCaseCmp( buffer, "-- Return" ) == 0 )
				StrCpyEx( buffer, "return", sizeof( buffer ) );
			else if( StrCaseCmp( buffer, "-- Refresh Page(here)" ) == 0 )
				StrCpyEx( buffer, "here", sizeof( buffer ) );

			if( configure_key_dispatch.button->pPageName )
				Release( configure_key_dispatch.button->pPageName );
			if( buffer[0] )
				configure_key_dispatch.button->pPageName = StrDup( buffer );
			else
				configure_key_dispatch.button->pPageName = NULL;
		}
	}
	// font was picked...
	if( configure_key_dispatch.new_font &&
		( !configure_key_dispatch.button->font_preset_name ||
		StrCaseCmp( configure_key_dispatch.new_font_name
		, configure_key_dispatch.button->font_preset_name ) ) )
	{
		if( configure_key_dispatch.button->font_preset_name )
			Release( (POINTER)configure_key_dispatch.button->font_preset_name );
		configure_key_dispatch.button->font_preset_name = configure_key_dispatch.new_font_name;
		configure_key_dispatch.new_font_name = NULL;
		configure_key_dispatch.button->font_preset = configure_key_dispatch.new_font;
		SetCommonFont( QueryGetControl( configure_key_dispatch.button )
			, (*configure_key_dispatch.button->font_preset ) );
	}
	else
	{
		if( configure_key_dispatch.new_font_name )
			Release( (POINTER)configure_key_dispatch.new_font_name );
	}
	// IsButtonChecked( GetControl( frame, BTN_BACK_IMAGE ) )
	if( ( pc = GetControl( frame, TXT_IMAGE_NAME) ) )
	{
		TEXTCHAR buf[256];
		GetControlText( pc
			, buf
			, sizeof( buf ) );
		InterShell_SetButtonImage( configure_key_dispatch.button, buf );
	}
	if( ( pc = GetControl( frame, TXT_IMAGE_V_MARGIN ) ) )
	{
		TEXTCHAR buf[256];
		GetControlText( pc
			, buf
						  , sizeof( buf ) );
		configure_key_dispatch.button->decal_vert_margin = atoi( buf );
	}
	if( ( pc = GetControl( frame, TXT_IMAGE_H_MARGIN ) ) )
	{
		TEXTCHAR buf[256];
		GetControlText( pc
			, buf
						  , sizeof( buf ) );
		configure_key_dispatch.button->decal_horiz_margin = atoi( buf );
	}

#ifndef __NO_ANIMATION__
	{
		TEXTCHAR buf[256];
		GetControlText( GetControl( frame, TXT_ANIMATION_NAME)
			, buf
			, sizeof( buf ) );
		InterShell_SetButtonAnimation( configure_key_dispatch.button, buf );
	}
#endif
	{
		PSI_CONTROL style_list = GetControl( frame, LST_BUTTON_STYLE );
		if( style_list )
		{
			PLISTITEM pli = GetSelectedItem( style_list );
			if( pli )
			{
				configure_key_dispatch.button->glare_set = (PGLARE_SET)GetItemData( pli );
			}
		}
	}
	{
		PSI_CONTROL name_field = GetControl( frame, TXT_CONTROL_TEXT );
		if( name_field )
		{
			TEXTCHAR text[256];
			GetControlText( name_field, text, sizeof( text ) );
			if( configure_key_dispatch.button->text )
				Release( configure_key_dispatch.button->text );
			configure_key_dispatch.button->text = StrDup( text );
		}
	}
	configure_key_dispatch.button->flags.bNoPress = GetCheckState( GetControl( frame, CHK_NOPRESS ) );
	SetButtonText( configure_key_dispatch.button );
	// only get this once.
}

//---------------------------------------------------------------------------
/*
void ApplyCommonButtonControls( PSI_CONTROL frame )
{
	// test for same frame?
	PMENU_BUTTON button = configure_key_dispatch.button;
	GetCommonButtonControls( frame );
	// restore this so we can get it again...
	configure_key_dispatch.button = button;
}
*/
//---------------------------------------------------------------------------

static void CPROC PickMenuControlFont( uintptr_t psv, PSI_CONTROL pc )
{
	SFTFont *font = SelectACanvasFont( configure_key_dispatch.canvas->pc_canvas
		, configure_key_dispatch.frame
		, &configure_key_dispatch.new_font_name
		);
	if( font )
		configure_key_dispatch.new_font = font;
}

//---------------------------------------------------------------------------

static void CPROC AddNewSystemName( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL list = GetNearControl( button, LIST_SYSTEMS );
	TEXTCHAR buffer[256];
	GetControlText( GetNearControl( button, EDIT_SYSTEM_NAME ), buffer, sizeof( buffer ) );
	if( !buffer[0] )
		return;
	AddListItem( list, buffer );

}

//---------------------------------------------------------------------------

static void CPROC AddSystemNameToAllow( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL list1, list2;
	PLISTITEM pli;
	TEXTCHAR buffer[256];
	list1 = GetNearControl( button, LIST_SYSTEMS );
	list2 = GetNearControl( button, LIST_ALLOW_SHOW );
	pli = GetSelectedItem( list1 );
	if( !pli )
	{
		// message, need select a system name
	}
	GetListItemText( pli, buffer, sizeof( buffer ) );
	SetItemData( AddListItem( list2, buffer ), GetItemData( pli ) );
}

//---------------------------------------------------------------------------

static void CPROC RemoveSystemNameAllow( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL list2;
	PLISTITEM pli;
	list2 = GetNearControl( button, LIST_ALLOW_SHOW );
	pli = GetSelectedItem( list2 );
	DeleteListItem( list2, pli );
}

//---------------------------------------------------------------------------

static void CPROC AddSystemNameToDisallow( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL list1, list2;
	TEXTCHAR buffer[256];
	PLISTITEM pli;
	list1 = GetNearControl( button, LIST_SYSTEMS );
	list2 = GetNearControl( button, LIST_DISALLOW_SHOW );
	pli = GetSelectedItem( list1 );
	if( !pli )
	{
		// message, need select a system name
	}
	GetListItemText( pli, buffer, sizeof( buffer ) );
	SetItemData( AddListItem( list2, buffer ), GetItemData( pli ) );

}

//---------------------------------------------------------------------------

static void CPROC RemoveSystemNameDisallow( uintptr_t psv, PSI_CONTROL button )
{
	PSI_CONTROL list2;
	PLISTITEM pli;
	list2 = GetNearControl( button, LIST_DISALLOW_SHOW );
	pli = GetSelectedItem( list2 );
	DeleteListItem( list2, pli );
}

//---------------------------------------------------------------------------

void SetAllowDisallowControls( void )
{
	// uses configure_key_dispatch informaation
	PSI_CONTROL frame = configure_key_dispatch.frame;
	PMENU_BUTTON button = configure_key_dispatch.button;
	PSI_CONTROL list;
	PSI_CONTROL list_sys;
	if( !button || button->flags.bInvisible )
		return;
	list = GetControl( frame, LIST_SYSTEMS );
	if( list )
	{
		struct system_info *system;
		list_sys = list;
		for( system = g.systems; system; system = NextThing( system ) )
		{
			AddListItem( list, GetText( system->name ) );
		}
	}
	else
		list_sys = NULL;
	list = GetControl( frame, LIST_ALLOW_SHOW );
	if( list )
	{
		INDEX idx;
		PTEXT name;
		LIST_FORALL( button->show_on, idx, PTEXT, name )
		{
			AddListItem( list, GetText( name ) );
			AddSystemName( list_sys, GetText( name ) );
		}
	}
	list = GetControl( frame, LIST_DISALLOW_SHOW );
	if( list )
	{
		INDEX idx;
		PTEXT name;
		LIST_FORALL( button->no_show_on, idx, PTEXT, name )
		{
			AddListItem( list, GetText( name ) );
			AddSystemName( list_sys, GetText( name ) );
		}
	}

	SetButtonPushMethod( GetControl( frame, BTN_ADD_SYSTEM ), AddNewSystemName, 0 );
	SetButtonPushMethod( GetControl( frame, BTN_ADD_SYSTEM_TO_DISALLOW ), AddSystemNameToDisallow, 0 );
	SetButtonPushMethod( GetControl( frame, BTN_ADD_SYSTEM_TO_ALLOW ), AddSystemNameToAllow, 0 );
	SetButtonPushMethod( GetControl( frame, BTN_REMOVE_SYSTEM_FROM_DISALLOW ), RemoveSystemNameDisallow, 0 );
	SetButtonPushMethod( GetControl( frame, BTN_REMOVE_SYSTEM_FROM_ALLOW ), RemoveSystemNameAllow, 0 );

}

/* this is function has a duplicately named function in pages.c */

static void CPROC MainChooseImage( uintptr_t psv, PSI_CONTROL pc )
{
	TEXTCHAR result[256];
	if( PSI_PickFile( pc, ".", NULL, result, sizeof( result ), FALSE ) )
	{
		SetControlText( GetNearControl( pc, TXT_IMAGE_NAME ), result );
	}
}
#ifndef __NO_ANIMATION__
static void CPROC ChooseAnimation( uintptr_t psv, PSI_CONTROL pc )
{
	TEXTCHAR result[256];
	if( PSI_PickFile( pc, ".", NULL, result, sizeof( result ), FALSE ) )
	{
		SetControlText( GetNearControl( pc, TXT_ANIMATION_NAME ), result );
	}
}
#endif

TEXTCHAR *I(uint32_t val)
{
	static TEXTCHAR buf[256];
	snprintf( buf, sizeof( buf ), "%ld", val );
	return buf;
}

void SetCommonButtonControls( PSI_CONTROL frame )
{
	// keep this, someday we might want to know it again...
	configure_key_dispatch.frame = frame;
	if( !configure_key_dispatch.button
		|| configure_key_dispatch.button->flags.bInvisible ) // nothing for this to do... nothing ocmmon about it.
	{
		HideControl( GetControl( frame, LST_BUTTON_STYLE ) );
		HideControl( GetControl( frame, BTN_PICKFONT ) );
		HideControl( GetControl( frame, LABEL_TEXT_COLOR) );
		HideControl( GetControl( frame, LABEL_BACKGROUND_COLOR) );
		HideControl( GetControl( frame, LABEL_RING_COLOR) );
		HideControl( GetControl( frame, LABEL_RING_HIGHLIGHT_COLOR ) );

		HideControl( GetControl( frame, CLR_TEXT_COLOR) );
		HideControl( GetControl( frame, CLR_RING_BACKGROUND) );
		HideControl( GetControl( frame, CLR_BACKGROUND) );
		HideControl( GetControl( frame, CLR_RING_HIGHLIGHT) );

		HideControl( GetControl( frame, TXT_IMAGE_NAME ) );
		HideControl( GetControl( frame, TXT_IMAGE_V_MARGIN ) );
		HideControl( GetControl( frame, TXT_IMAGE_H_MARGIN ) );
#ifndef __NO_ANIMATION__
		HideControl( GetControl( frame, TXT_ANIMATION_NAME ) );
#endif
		HideControl( GetControl( frame, TXT_CONTROL_TEXT ) );
		HideControl( GetControl( frame, CHK_NOPRESS ) );

		//HideControl( GetControl( frame, LST_PAGES ) );

		HideControl( GetControl( frame, LIST_ALLOW_SHOW ) );
		HideControl( GetControl( frame, LIST_DISALLOW_SHOW ) );
		HideControl( GetControl( frame, LIST_SYSTEMS ) );
		HideControl( GetControl( frame, LIST_ALLOW_SHOW ) );
		HideControl( GetControl( frame, BTN_ADD_SYSTEM ) );
		HideControl( GetControl( frame, BTN_ADD_SYSTEM_TO_DISALLOW ) );
		HideControl( GetControl( frame, BTN_ADD_SYSTEM_TO_ALLOW ) );

		//return;
	}
	else
	{
		if( !configure_key_dispatch.button->flags.bListbox )
		{
			PSI_CONTROL style_list = GetControl( frame, LST_BUTTON_STYLE );
			if( style_list )
			{
				PGLARE_SET glare_set;
				INDEX idx;
				LIST_FORALL( g.glare_sets, idx, PGLARE_SET, glare_set )
				{
					PLISTITEM pli = AddListItem( style_list, glare_set->name );
					SetItemData( pli, (uintptr_t)glare_set );
					if( StrCaseCmp( configure_key_dispatch.button->glare_set->name
								 , glare_set->name ) == 0 )
					{
						SetSelectedItem( style_list, pli );
					}
				}
			}
		}
		else
			HideControl( GetControl( frame, LST_BUTTON_STYLE ) );

		if( configure_key_dispatch.button->font_preset_name )
			configure_key_dispatch.new_font_name = StrDup( configure_key_dispatch.button->font_preset_name );
		else
			configure_key_dispatch.new_font_name = NULL;
		configure_key_dispatch.new_font = NULL;
		SetButtonPushMethod( GetControl( frame, BTN_PICKFONT ), PickMenuControlFont, 0 );
		SetButtonPushMethod( GetControl( frame, BTN_PICKFILE ), MainChooseImage, 0 );
#ifndef __NO_ANIMATION__
		SetButtonPushMethod( GetControl( frame, BTN_PICKANIMFILE ), ChooseAnimation, 0 );
#endif
		SetAllowDisallowControls();
		SetupSecurityEdit( frame, configure_key_dispatch.button->psvUser );

		{
			struct menu_button_colors *colors = (struct menu_button_colors*)GetLink( &configure_key_dispatch.button->colors, g.theme_index );
			if( !colors )
				colors = (struct menu_button_colors*)GetLink( &configure_key_dispatch.button->colors, 0 );
			EnableColorWellPick( SetColorWell( GetControl( frame, CLR_TEXT_COLOR), colors->textcolor ), TRUE );
			EnableColorWellPick( SetColorWell( GetControl( frame, CLR_RING_BACKGROUND), colors->secondary_color ), TRUE );
			EnableColorWellPick( SetColorWell( GetControl( frame, CLR_BACKGROUND), colors->color ), TRUE );

			if( !configure_key_dispatch.button->flags.bListbox &&
				!configure_key_dispatch.button->flags.bCustom )
			{
				EnableColorWellPick( SetColorWell( GetControl( frame, CLR_RING_HIGHLIGHT), colors->highlight_color ), TRUE );
				SetControlText( GetControl( frame, TXT_IMAGE_NAME ), configure_key_dispatch.button->pImage );
				SetControlText( GetControl( frame, TXT_IMAGE_H_MARGIN ), I(configure_key_dispatch.button->decal_horiz_margin) );
				SetControlText( GetControl( frame, TXT_IMAGE_V_MARGIN ), I(configure_key_dispatch.button->decal_horiz_margin) );

#ifndef __NO_ANIMATION__
				SetControlText( GetControl( frame, TXT_ANIMATION_NAME ), configure_key_dispatch.button->pAnimation );
#endif
				if( configure_key_dispatch.button->text )
				{
					SetControlText( GetControl( frame, TXT_CONTROL_TEXT ), configure_key_dispatch.button->text );
				}
				else
					SetControlText( GetControl( frame, TXT_CONTROL_TEXT ), "" );

				SetCheckState( GetControl( frame, CHK_NOPRESS )
								 , configure_key_dispatch.button->flags.bNoPress );
			}
		}
	}
	// startup and shutdown macros are crazy.
	if( configure_key_dispatch.button )
		{
			PCanvasData canvas = GetCanvas( GetParentControl( QueryGetControl( configure_key_dispatch.button ) ) );
			PSI_CONTROL list = GetControl( frame, LST_PAGES );
			if( canvas && list )
			{
				INDEX idx;
				PPAGE_DATA page;
				PLISTITEM pli;
				pli = AddListItem( list, "-- NONE -- " );
				if( !configure_key_dispatch.button->pPageName )
					SetSelectedItem( list, pli );
				pli = AddListItem( list, "-- Startup Page" );
				if( configure_key_dispatch.button->pPageName &&
					StrCaseCmp( configure_key_dispatch.button->pPageName, "first" ) == 0 )
					SetSelectedItem( list, pli );
				pli = AddListItem( list, "-- Next" );
				if( configure_key_dispatch.button->pPageName &&
					StrCaseCmp( configure_key_dispatch.button->pPageName, "next" ) == 0 )
					SetSelectedItem( list, pli );

				pli = AddListItem( list, "-- Return" );
				if( configure_key_dispatch.button->pPageName &&
					StrCaseCmp( configure_key_dispatch.button->pPageName, "return" ) == 0 )
					SetSelectedItem( list, pli );

				pli = AddListItem( list, "-- Refresh Page(here)" );
				if( configure_key_dispatch.button->pPageName &&
					StrCaseCmp( configure_key_dispatch.button->pPageName, "here" ) == 0 )
					SetSelectedItem( list, pli );

				LIST_FORALL( canvas->pages, idx, PPAGE_DATA, page )
				{
					pli = AddListItem( list, page->title );
					if( configure_key_dispatch.button->pPageName &&
						StrCaseCmp( configure_key_dispatch.button->pPageName, page->title ) == 0 )
					{
						SetSelectedItem( list, pli );
					}
				}
			}
		}
}

//---------------------------------------------------------------------------

static void editButtonResult( uintptr_t psv, PSI_CONTROL pc, int done, int okay ) {
	if( okay )
	{
		GetCommonButtonControls( pc );
	}
	DestroyFrame( &pc );
}

// uses the currently selected button...
static void InterShell_EditButton( PSI_CONTROL pc_parent )
{

	PSI_CONTROL frame = LoadXMLFrameOver( pc_parent, "EditGenericButton.isframe" ); // can use this frame also, just default controls
	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetCommonButtons( frame, &done, &okay );
		SetCommonButtonControls( frame );  // magically knows which button we're editing at the moment.
		DisplayFrameOver( frame, pc_parent );
		//EditFrame( frame, TRUE );
		PSI_HandleStatusEvent( frame, glareSetResult, 0 );
	}
	//return psv;

}

//---------------------------------------------------------------------------

// uses the currently selected button...
static void InterShell_EditGeneric( PSI_CONTROL pc_parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( pc_parent, "EditGenericControl.isframe" ); // can use this frame also, just default controls
	if( frame )
	{
		int okay = 0;
		int done = 0;
		SetCommonButtons( frame, &done, &okay );
		SetCommonButtonControls( frame );  // magically knows which button we're editing at the moment.
		DisplayFrameOver( frame, pc_parent );
		EditFrame( frame, TRUE );
		PSI_HandleStatusEvent( frame, glareSetResult, 0 );
	}
}

//---------------------------------------------------------------------------

static void listboxSetResult( uintptr_t psv, PSI_CONTROL pc, int done, int okay ) {
	if( okay ) {
		PSI_CONTROL list = (PSI_CONTROL)psv;
		PSI_CONTROL pc;
		GetCommonButtonControls( frame );
		pc = GetControl( frame, CHECKBOX_LIST_LAZY_MULTI_SELECT );
		if( pc )
		{
			SetListboxMultiSelectEx( list, GetCheckState( GetControl( frame, CHECKBOX_LIST_MULTI_SELECT ) ), GetCheckState( pc ) );
		}
		else
		{
			SetListboxMultiSelect( list, GetCheckState( GetControl( frame, CHECKBOX_LIST_MULTI_SELECT ) ) );
		}
	}
	DestroyFrame( &pc );
}

// uses the currently selected button...
static void InterShell_EditListbox( PSI_CONTROL pc_parent )
{
	PSI_CONTROL frame = LoadXMLFrameOver( pc_parent, "EditGenericListbox.isframe" ); // can use this frame also, just default controls
	if( frame )
	{
		PSI_CONTROL list = (PSI_CONTROL)configure_key_dispatch.button->control.control;
		int okay = 0;
		int done = 0;
		SetCommonButtons( frame, &done, &okay );
		SetCommonButtonControls( frame );  // magically knows which button we're editing at the moment.
		{
			int multi, lazy;
			GetListboxMultiSelectEx( list, &multi, &lazy );
			SetCheckState( GetControl( frame, CHECKBOX_LIST_MULTI_SELECT ), multi );
			SetCheckState( GetControl( frame, CHECKBOX_LIST_LAZY_MULTI_SELECT ), lazy );
		}
		DisplayFrameOver( frame, pc_parent );
		EditFrame( frame, TRUE );
		PSI_HandleStatusEvent( frame, listboxSetResult, (uintptr_t)list );
	}
	//return psv;
}

static void CloneVisuals( PMENU_BUTTON copy_from, PMENU_BUTTON copy_to )
{
	struct menu_button_colors *copy_from_colors = (struct menu_button_colors*)GetLink( &copy_from->colors, g.theme_index );
	struct menu_button_colors *copy_to_colors = (struct menu_button_colors*)GetLink( &copy_to->colors, g.theme_index );
	if( !copy_from_colors )
		copy_from_colors = (struct menu_button_colors*)GetLink( &copy_from->colors, 0 );
	if( !copy_to_colors )
		copy_to_colors = (struct menu_button_colors*)GetLink( &copy_to->colors, 0 );

	copy_to->flags.bConfigured = copy_from->flags.bConfigured;
	copy_to->glare_set = copy_from->glare_set;
	copy_to_colors->color = copy_from_colors->color;
	copy_to_colors->secondary_color = copy_from_colors->secondary_color;
	copy_to_colors->textcolor = copy_from_colors->textcolor;
	copy_to_colors->highlight_color = copy_from_colors->highlight_color;
	InterShell_SetButtonFontName( copy_to, copy_from->font_preset_name );
}

//---------------------------------------------------------------------------

struct configure_info
{
	struct {
		BIT_FIELD complete : 1;
		BIT_FIELD received : 1;
		BIT_FIELD bIgnorePrivate : 1;
	} flags;
	PTHREAD waiting; // if waiting, flags.complete will be set, and this thread will wake.
	PSI_CONTROL parent;
	PMENU_BUTTON button;
	PCanvasData canvas;
};

uintptr_t CPROC ThreadConfigureButton( PTHREAD thread )
{
	struct configure_info *info = (struct configure_info *)GetThreadParam( thread );
	struct configure_key_dispatch save; // other child windows cannot complete.
	//PSI_CONTROL parent = info->parent;
	PCanvasData canvas	  = info->canvas;
	int bIgnorePrivate	  = info->flags.bIgnorePrivate;
	PSI_CONTROL pc_parent  = info->parent;
	PMENU_BUTTON button	 = (PMENU_BUTTON)info->button;
	PMENU_BUTTON prioredit = configure_key_dispatch.button;
	PTHREAD wake			  = info->waiting;

	info->flags.received = 1;
	// do not use (*info) after this point!
	// the calling thread is GONE and so is this info.
	save = configure_key_dispatch;
	MemSet( &configure_key_dispatch, 0, sizeof( configure_key_dispatch ) );

	{
		TEXTCHAR rootname[256];
		uintptr_t (CPROC*f)(uintptr_t,PSI_CONTROL);
		while( !button->flags.bInvisible && configure_key_dispatch.button )
			IdleFor(100);
		configure_key_dispatch.canvas = canvas;
		configure_key_dispatch.button = button;
		snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
		if( !bIgnorePrivate )
		{
			f = GetRegisteredProcedure2( rootname, uintptr_t, "control_edit", (uintptr_t,PSI_CONTROL) );
			if( f )
			{
				button->psvUser = f(button->psvUser, pc_parent );
				FixupButton( button );
			}
		}
		else
			f = NULL;
		if( !f )
		{
			// depricated method...
			if( !bIgnorePrivate )
			{
				snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/control/%s", button->pTypeName );
				f = GetRegisteredProcedure2( rootname, uintptr_t, "button_edit", (uintptr_t,PSI_CONTROL) );
				if( f )
				{
					button->psvUser = f(button->psvUser, pc_parent );
					FixupButton( button );
				}
			}
			if( !f )
			{
				// if it's a standard button... attempt to edit standard properties on it.
				if( button->flags.bListbox )
					InterShell_EditListbox( pc_parent );
				else if( !button->flags.bCustom )
					InterShell_EditButton( pc_parent );
				else if( button->flags.bCustom )
					InterShell_EditGeneric( pc_parent );
			}
		}
		configure_key_dispatch.button = prioredit;
	}
	configure_key_dispatch = save;
	// done with this...
	if( canvas )
	{
		if( !configure_key_dispatch.button )
		{
			canvas->flags.bIgnoreKeys = 0;
		}
		else
		{
			DebugBreak();
		}
		if( canvas->nSelected )
		{
			INDEX idx;
			PMENU_BUTTON item;
			LIST_FORALL( canvas->selected_list, idx, PMENU_BUTTON, item )
			{
				if( button == item )
				{
					break;
				}
			}
			// found item in the list of selected controls, so...
			if( item )
			{
				LIST_FORALL( canvas->selected_list, idx, PMENU_BUTTON, item )
				{
					if( button == item )
						continue;
					CloneVisuals( button, item );
				}
			}
		}

	}
	if( wake )
	{
		// this parameter means the calling thread is waiting for completion, thereofre info is still a valid pointer.
		info->flags.complete = 1;
		WakeThread( wake );
	}

	return 0;
}

//---------------------------------------------------------------------------

void ConfigureKeyExx( PSI_CONTROL parent, PMENU_BUTTON button, int bWaitComplete, int bIgnoreControlOverload )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, parent );
	struct configure_info info;
	info.parent = parent;
	info.button = button;
	info.canvas = canvas;
	info.flags.received = 0;
	info.flags.bIgnorePrivate = bIgnoreControlOverload;
	if( bWaitComplete )
		info.waiting = MakeThread();
	else
		info.waiting = NULL;
	// okay yeah done with that key...
	if( ( button && !button->flags.bInvisible ) && canvas->flags.bIgnoreKeys )
		return;
	if( canvas )
		canvas->flags.bIgnoreKeys = 1;

	ThreadTo( ThreadConfigureButton, (uintptr_t)&info );
	// allow thread to read parameters from info structure.
	while( !info.flags.received )
		Relinquish();
	if( bWaitComplete )
		while( !info.flags.complete )
			IdleFor( 250 );
}

void ConfigureKeyEx( PSI_CONTROL parent, PMENU_BUTTON button )
{
	ConfigureKeyExx( parent, button, FALSE, FALSE );
}
//---------------------------------------------------------------------------

int ProcessPageMenuResult( PSI_CONTROL pc_canvas, uint32_t result )
{
	if( result >= MNU_GLOBAL_PROPERTIES && result <= MNU_GLOBAL_PROPERTIES_MAX )
	{
		ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
		void (CPROC*f)(PSI_CONTROL);
		f = (void (CPROC*)(PSI_CONTROL) )GetLink( &g.global_properties, result - MNU_GLOBAL_PROPERTIES );
		if( f )
		{
			f( canvas->edit_glare_frame?canvas->edit_glare_frame:canvas->current_page->frame );
		}
		return 1;
	}
	if( result >= MNU_CHANGE_PAGE && result <= MNU_CHANGE_PAGE_MAX )
	{
		SetCurrentPageID( pc_canvas, result - MNU_CHANGE_PAGE );
		return 1;
	}
	if( result >= MNU_DESTROY_PAGE && result <= MNU_DESTROY_PAGE_MAX )
	{
		DestroyPageID( pc_canvas, result - MNU_DESTROY_PAGE  );
		return 1;
	}
	if( result >= MNU_UNDELETE_PAGE && result <= MNU_UNDELETE_PAGE_MAX )
	{
		// move from the undelete menu to the real page menu
		UnDestroyPageID( pc_canvas, result - MNU_DESTROY_PAGE  );
		return 1;
	}
	return 0;
}

void CloneCommonButtonProperties( PMENU_BUTTON clone, PMENU_BUTTON  clonebutton )
{
	struct menu_button_colors *clone_colors = (struct menu_button_colors*)GetLink( &clone->colors, g.theme_index );
	struct menu_button_colors *clone_from_colors = (struct menu_button_colors*)GetLink( &clonebutton->colors, g.theme_index );
	if( !clone_from_colors )
		clone_from_colors = (struct menu_button_colors*)GetLink( &clonebutton->colors, 0 );
	if( !clone_colors )
		clone_colors = (struct menu_button_colors*)GetLink( &clone->colors, 0 );

	clone_colors->color			  = clone_from_colors->color;
	clone_colors->secondary_color = clone_from_colors->secondary_color;
	clone_colors->textcolor		 = clone_from_colors->textcolor;
	clone_colors->highlight_color = clone_from_colors->highlight_color;
	clone->font_preset	  = clonebutton->font_preset;
	clone->font_preset_name = clonebutton->font_preset_name;
	clone->text				= StrDup( clonebutton->text );
	StrCpyEx( clone->pImage, clonebutton->pImage, sizeof( clone->pImage ) );
#ifndef __NO_ANIMATION__
	strcpy( clone->pAnimation, clonebutton->pAnimation );
#endif
	clone->flags.bNoPress  = clonebutton->flags.bNoPress;
	clone->flags.bIgnorePageChange= clonebutton->flags.bIgnorePageChange;
	clone->flags.bSecure	= clonebutton->flags.bSecure;
	//clone->flags.bSecureEndContext ;
	clone->pPageName		 = StrDup( clonebutton->pPageName );
	clone->glare_set		 = clonebutton->glare_set; // glares used on this button

	// if this context is already entered, then the security check is not done.
	//TEXTSTR security_context; // once entered, this context is set...
	//TEXTSTR security_reason; // reason to log into permission_user_info
	//TEXTSTR security_token; // filter list of users by these tokens, sepeareted by ','
	//INDEX iSecurityContext; // index into login_history that identifies the context of this login..
}

void InterShell_SetCloneButton( PMENU_BUTTON button )
{
	g.clonebutton = button;
}

PMENU_BUTTON GetCloneButton( PCanvasData canvas, int px, int py, int bInvisible )
{
	if( g.clonebutton )
	{
		// commented bit creates new controled centered on mouse
		PMENU_BUTTON clone;
		if( bInvisible || !canvas )
			clone = CreateInvisibleControl( canvas->pc_canvas, g.clonebutton->pTypeName );
		else
		{
			lprintf( "This is where cloned controls are created." );
			clone = CreateSomeControl( canvas->current_page->frame
											 , px //- (g.clonebutton->w/2)
											 , py //- (g.clonebutton->h/2)
											 , (uint32_t)(g.clonebutton->w?g.clonebutton->w:4)
											 , (uint32_t)(g.clonebutton->h?g.clonebutton->h:2)
											 , g.clonebutton->pTypeName );
		}
		CloneCommonButtonProperties( clone, g.clonebutton );
		InvokeCloneControl( clone, g.clonebutton );
		SmudgeCommon( QueryGetControl( clone ) );
		return clone;
	}
	return NULL;
}

//---------------------------------------------------------------------------
int CPROC MouseEditGlare( uintptr_t psv, int32_t x, int32_t y, uint32_t b );

static int OnMouseCommon( "Menu Canvas" )( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );
	static uint32_t _b;
	static int32_t _x, _y;
	//int px, py;
#define PARTOFX(xc) ( ( xc ) * canvas->current_page->grid.nPartsX ) / canvas->width
#define PARTOFY(yc) ( ( yc ) * canvas->current_page->grid.nPartsY ) / canvas->height
	//px = PARTOFX( x );
	//py = PARTOFY( y );
	if( canvas->flags.bEditMode )
	{
		return MouseEditGlare( (uintptr_t)canvas, x, y, b );
	}
	// shell mouse is frame mouse?
	//lprintf( "Shell mosue %d,%d %d", x, y,  b );

	_b = b;
	_x = x;
	_y = y;
	// this didn't really use this ....
	return 0;
}


//---------------------------------------------------------------------------

#ifdef USE_EDIT_GLARE
void CPROC DrawEditGlare( uintptr_t psv, PRENDERER edit_glare )
#else
void CPROC DrawEditGlare( uintptr_t psv, Image surface )
#endif
{
	{
		PCanvasData canvas = (PCanvasData)psv;
		//uint32_t w, h;
		PPAGE_DATA current_page = canvas->current_page;

		{
			// should use X to capture an image of the current menu config
			// then during editing we do not have to redraw all controls all the time..
			BlatColorAlpha( surface, 0, 0, surface->width, surface->height, 0x20101010 );

			{
				PMENU_BUTTON button;
				INDEX idx;
				int nButton = 1;
				CDATA base_color;
				LIST_FORALL( current_page->controls, idx, PMENU_BUTTON, button )
				{
					int selected = 0;
					long long x, y;
					long long w, h;
					TEXTCHAR buttonname[128];
					{
						INDEX idx2;
						PMENU_BUTTON button2;
						base_color = BASE_COLOR_GREEN;
						LIST_FORALL( canvas->selected_list, idx2, PMENU_BUTTON, button2 )
						{
							if( button2 == button )
							{
								selected = 1;
								base_color = BASE_COLOR_MAGENTA;
								break;
							}
						}
					}
					if( !selected )
						if( button == canvas->pCurrentControl )
							selected = 1;
					//lprintf( "Drawing at %d,%d...", button->x, button->y );
					x = (PARTX( button->x ) + 5); y = (PARTY( button->y ) + 5);
					w=(PARTW( button->x, button->w )+1 - 10);
					h=(PARTH( button->y, button->h )+1 - 10);
					if( x < 0 )
					{
						w += x;
						x = 0;
					}
					if( y < 0 )
					{
						h += y;
						y = 0;
					}
					//lprintf( "region is %Ld,%Ld %Ld,%Ld", x, y, w, h) ;
					BlatColorAlpha( surface, (int32_t)x, (int32_t)y, (uint32_t)w, (uint32_t)h
						, SetAlpha( (selected
									&&canvas->flags.dragging)?BASE_COLOR_YELLOW:base_color
									, (button==canvas->pCurrentControl
									&&canvas->flags.sizing_by_corner == DRAG_BEGIN)?180:90 )
									  );
					w = PARTW(button->x,1);
					h = PARTH(button->y,1);
					BlatColorAlpha( surface
									  , PARTX(button->x), PARTY(button->y)
									  , (uint32_t)(w < 20?20:w), (uint32_t)(h<20?20:h)
									  , SetAlpha( (button==canvas->pCurrentControl
														&&canvas->flags.sizing)?BASE_COLOR_YELLOW:BASE_COLOR_RED
													, ( button==canvas->pCurrentControl
														&&canvas->flags.sizing_by_corner == UPPER_LEFT)?180:90 )
									  );
					w = PARTW(button->x,1);
					h = PARTH(button->y+button->h,1);
					BlatColorAlpha( surface
									  , (int32_t)PARTX(button->x), (int32_t)(PARTY(button->y+button->h) - (h<20?20:h))
									  , (uint32_t)(w < 20?20:w), (uint32_t)(h<20?20:h)
									  , SetAlpha( (button==canvas->pCurrentControl
														&&canvas->flags.sizing)?BASE_COLOR_YELLOW:BASE_COLOR_RED, (button==canvas->pCurrentControl
																																	  &&canvas->flags.sizing_by_corner == LOWER_LEFT)?180:90 ) );
					w = PARTW(button->x+button->w,1);
					h = PARTH(button->y,1);
					BlatColorAlpha( surface
									  , (int32_t)(PARTX(button->x+button->w) - (w < 20?20:w)), (int32_t)PARTY(button->y)
									  , (uint32_t)(w < 20?20:w), (uint32_t)(h<20?20:h)
									  , SetAlpha( (button==canvas->pCurrentControl
														&&canvas->flags.sizing)?BASE_COLOR_YELLOW:BASE_COLOR_RED, (button==canvas->pCurrentControl
																																	  &&canvas->flags.sizing_by_corner == UPPER_RIGHT)?180:90 ) );
					w = PARTW(button->x+button->w,1);
					h = PARTH(button->y+button->h-1,1);
					BlatColorAlpha( surface
									  , (int32_t)(PARTX(button->x+button->w) - (w < 20?20:w) ), (int32_t)(PARTY(button->y+button->h) - (h<20?20:h))
									  , (uint32_t)(w < 20?20:w), (uint32_t)(h<20?20:h)
									  , SetAlpha( (button==canvas->pCurrentControl
														&&canvas->flags.sizing)?BASE_COLOR_YELLOW:BASE_COLOR_RED, (button==canvas->pCurrentControl
																																	  &&canvas->flags.sizing_by_corner == LOWER_RIGHT)?180:90 ) );
					snprintf( buttonname, sizeof( buttonname ), "%s(%d)"
							  , button->pTypeName
							  , nButton++
							  );
					PutString( surface
								, (int32_t)x + 15, (int32_t)y + 15, 0, BASE_COLOR_WHITE, 0
								, buttonname );
					{
						TEXTCHAR position_text[25];
						snprintf( position_text, sizeof( position_text ), "%lld,%lld", button->x, button->y );
						PutString( surface
									, (int32_t)x + 15, (int32_t)y + 28, 0, BASE_COLOR_WHITE, 0
									, position_text );
						snprintf( position_text, sizeof( position_text ), "%lldx%lld", button->w, button->h );
						PutString( surface
									, (int32_t)x + 15, (int32_t)y + 41, 0, BASE_COLOR_WHITE, 0
									, position_text );

					}
				}
			}

			{
				int last_x = -1, last_y = -1;
				int x, y;

				for( x = 0; x < PARTSX; x++ )
				{
					//lprintf( "line %d...", x );
					if( (last_x > 0) && ((PARTX(x) - last_x) < 15) )
					{
						//lprintf( "skipping. %d %d", last_x, PARTX(x) );
						continue;
					}
					last_x = PARTX(x);
					//lprintf( "Line %d has %d", x, PARTX(x) );
					do_vlineAlpha( surface, PARTX(x), 0, surface->height, SetAlpha( BASE_COLOR_WHITE, 48 ) );
				}

				for( y = 0; y < PARTSY; y++ )
				{
					//lprintf( "line %d...", y );
					if( (last_y > 0) && ((PARTY(y) - last_y) < 15) )
					{
						//lprintf( "skipping. %d %d", last_y, PARTY(y) );
						continue;
					}
					last_y = PARTY(y);
					//lprintf( "Line %d has %d", y, PARTY(y) );
					do_hlineAlpha( surface, PARTY(y), 0, surface->width, SetAlpha( BASE_COLOR_WHITE, 48 ) );
				}

				if( canvas->flags.selected )
				{
					long long x, y;
					//lprintf( "Our fancy coords could be %d,%d %d,%d", PARTX( canvas->selection.x ), PARTY( canvas->selection.y )
					//		 , PARTW( canvas->selection.x, canvas->selection.w )
					//		 , PARTH( canvas->selection.y, canvas->selection.h ));
					// and to look really pretty select the outer edge on the bottom, also
					BlatColorAlpha( surface
									  , (int32_t)(x = PARTX( canvas->selection.x ) )
									  , (int32_t)(y = PARTY( canvas->selection.y ) )
									  , (uint32_t)PARTW( canvas->selection.x, canvas->selection.w )+1
									  , (uint32_t)PARTH( canvas->selection.y, canvas->selection.h )+1
									  , canvas->flags.selecting
										?SetAlpha( ColorAverage( BASE_COLOR_BLUE, BASE_COLOR_WHITE, 50, 100 ), 170 )
										:SetAlpha( BASE_COLOR_BLUE, 120 )
									  );
					{
						TEXTCHAR position_text[25];
						snprintf( position_text, sizeof( position_text ), "%d,%d", canvas->selection.x, canvas->selection.y );
						PutString( surface
									, (int32_t)x + 15, (int32_t)y + 28, 0, BASE_COLOR_WHITE, 0
									, position_text );
						snprintf( position_text, sizeof( position_text ), "%dx%d", canvas->selection.w, canvas->selection.h );
						PutString( surface
									, (int32_t)x + 15, (int32_t)y + 41, 0, BASE_COLOR_WHITE, 0
									, position_text );

					}
				}

			}
		}
	}
#ifdef USE_EDIT_GLARE
	UpdateDisplay( edit_glare );
#endif
}


//---------------------------------------------------------------------------

static int OnDrawCommon( "Menu Canvas" )( PSI_CONTROL pf )
{
#ifdef DEBUG_BACKGROUND_UPDATE
	lprintf( "got Draw..." );
	lprintf( "----------g.flags.bPageUpdateDisabled %d", g.flags.bPageUpdateDisabled );
#endif
	if( !g.flags.bPageUpdateDisabled )
	{
		ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pf );
		PPAGE_DATA current_page;
		//uint32_t w, h;
		Image surface = GetFrameSurface( pf );
		//GetPageSize( pf, &w, &h );
		//if( g.flags.multi_edit )
		current_page = canvas->current_page;
		//lprintf( "--- AM DRAWING BACKGROUND" );
		// update the canvas's dimensions...
		////////-s-s-s-s
		if( (( canvas->width != surface->width )&& canvas->width ) ||
			(( canvas->height != surface->height )&& canvas->height ) )
		{
			INDEX idx;
			PMENU_BUTTON button;
			INDEX idx_page;
			PPAGE_DATA page;
			lprintf( "Display size changed.... %d,%d to %d,%d",canvas->width, canvas->height, surface->width, surface->height  );

			if( canvas->edit_glare )
			{
				int32_t x, y;
				uint32_t w, h;
				GetDisplayPosition( canvas->renderer, &x, &y, &w, &h );
				MoveSizeDisplay( canvas->edit_glare, x, y, w, h );
			}
			canvas->width = surface->width;
			canvas->height = surface->height;
			// just update the numerator, the denomintor is the same.
			canvas->width_scale.numerator =  surface->width;
			canvas->height_scale.numerator = surface->height;

			UpdateFontScaling( canvas );

			LIST_FORALL( canvas->pages, idx_page, PPAGE_DATA, page )
			{
				LIST_FORALL( page->controls, idx, PMENU_BUTTON, button )
				{
					if( button->font_preset )
						SetCommonFont( QueryGetControl( button ), (*button->font_preset) );
					MoveSizeCommon( QueryGetControl( button )
						, PARTX( button->x )
						, PARTY( button->y )
						, PARTW( button->x, button->w )
						, PARTH( button->y, button->h )
						);
				}
			}
			{
				LIST_FORALL( canvas->default_page->controls, idx, PMENU_BUTTON, button )
				{
					if( button->font_preset )
						SetCommonFont( QueryGetControl( button ), (*button->font_preset) );
					MoveSizeCommon( QueryGetControl( button )
						, PARTX( button->x )
						, PARTY( button->y )
						, PARTW( button->x, button->w )
						, PARTH( button->y, button->h )
						);
				}
			}
		}
		//else
		//	current_page = g.current_page;
		//EnableFrameUpdates( pf, FALSE );
		//DumpFrameContents( pf );
		if( current_page )
		{
			if( !current_page->background_image && current_page->background )
			{
				current_page->background_image = LoadImageFileFromGroup( 0, (TEXTCHAR*)current_page->background );
#ifdef DEBUG_BACKGROUND_UPDATE
				lprintf( "Loaded image is %p %s", current_page->background_image, current_page->background );
#endif
			}
			if( current_page->background_color )
				ClearImageTo( surface, current_page->background_color );
			if( current_page->background_image )
			{
				// hmm - hazy edges will be bad... need to
				// option to fix the constant background shade or not...
#ifdef DEBUG_BACKGROUND_UPDATE
				xlprintf(LOG_UPDATE_AND_REFRESH_LEVEL)( "-------------------------------------- Draw Background Image -------------------------------------" );
#endif
				BlotScaledImageSizedToAlpha( surface, current_page->background_image
					, 0, 0
					, surface->width, surface->height
					, ALPHA_TRANSPARENT );
			}
			else if( !current_page->background_color ) // otherwise it's transparent black. (dark clear)
			{
#ifdef DEBUG_BACKGROUND_UPDATE
				xlprintf(LOG_UPDATE_AND_REFRESH_LEVEL)( "-------------------------------------- Draw Background Color -------------------------------------" );
#endif
				if( canvas->flags.bEditMode )
					ClearImageTo( surface, 0x01000000 );//BASE_COLOR_BLACK );
				else
					ClearImage( surface );
			}
#ifdef DEBUG_BACKGROUND_UPDATE
			else
			{
				xlprintf(LOG_UPDATE_AND_REFRESH_LEVEL)( "-------------------------------------- NO Draw Background -------------------------------------" );
			}
#endif
		}
		else
			DebugBreak();

		{
			PMENU_BUTTON button;
			INDEX idx;
			int controls = 0;
			if( current_page ) {
				LIST_FORALL( current_page->controls, idx, PMENU_BUTTON, button )
				{
					controls++;
				}
				if( controls == 0 )
				{
					SFTFont *font = UseACanvasFont( pf, "Default" );
					int y = 15;
					int skip = GetFontHeight( (*font ) );
					PutStringFont( surface
								, (int32_t)15, (int32_t)y, 0, BASE_COLOR_WHITE, SetAlpha( BASE_COLOR_BLACK, 90 )
								, "There are no controls defined...", (*font) );
					//y += skip;
					//PutStringFont( surface
					//			, (int32_t)15, (int32_t)y, BASE_COLOR_WHITE, SetAlpha( BASE_COLOR_BLACK, 90 )
					//				 , "Press Alt-C to edit"
					//				 , (*font)
					//				 );
					//y += skip;
					//PutStringFont( surface
					//			, (int32_t)15, (int32_t)y, BASE_COLOR_WHITE, SetAlpha( BASE_COLOR_BLACK, 90 )
					//				 , "Right click on empty space to edit other properties and add plugins..."
					//				 , (*font)
					//				 );
				}
			} else {
				SFTFont *font = UseACanvasFont( pf, "Default" );
				int y = 15;
				int skip = GetFontHeight( (*font ) );
				PutStringFont( surface
							, (int32_t)15, (int32_t)y, 0, BASE_COLOR_WHITE, SetAlpha( BASE_COLOR_BLACK, 90 )
							, "There are no pages defined!!!", (*font) );

			}
		}

#ifndef USE_EDIT_GLARE
		// if really using the galre, the glare is a transparent overlayer
		if( canvas->flags.bEditMode )
		{
			DrawEditGlare( (uintptr_t)canvas, surface );
		}
#endif
	}
	//lprintf( "Done with a pass of drawing the canvas background... but the controls are still being revealed oh when?" );
	return TRUE;
}

//---------------------------------------------------------------------------
struct popupParams {
	PCanvasData canvas;
	PSI_CONTROL pc;
	int32_t px, py;
	PSI_CONTROL parent_frame;

	int read;
};

static void selectedRegionCallback( uintptr_t psv, int result ) {
	struct popupParams* params = (struct popupParams*)psv;
	PSI_CONTROL pc = params->pc;
	int32_t px = params->px;
	int32_t py = params->py;
	PSI_CONTROL parent_frame = params->parent_frame;

	PCanvasData canvas = params->canvas;
	params->read = 1;
#ifdef USE_EDIT_GLARE
					OwnMouse( canvas->edit_glare, TRUE );
#endif

					if( g.flags.multi_edit )
					{
						PPAGE_DATA page = GetPageFromFrame( pc );
						canvas->current_page = page;
					}
					if( !ProcessPageMenuResult( pc, result ) )
					{
						if( result >= MNU_CREATE_EXTRA && result <= MNU_CREATE_EXTRA_MAX )
						{
							TEXTCHAR *name;
							// okay well... get the name from the menu item then?
							result -= MNU_CREATE_EXTRA;
							name = (TEXTCHAR*)GetLink( &g.extra_types, result );
							canvas->flags.selected = 0;
							CreateSomeControl( pc, canvas->selection.x, canvas->selection.y, canvas->selection.w, canvas->selection.h, name );
						}
						else switch( result )
						{
						case MNU_CREATE_ISSUE:
							CreateSomeControl( pc, canvas->selection.x, canvas->selection.y, canvas->selection.w, canvas->selection.h, "Paper Issue" );
							break;
						case MNU_CREATE_CONTROL:
							//CreateMenuControl( g.selection.x, g.selection.y, canvas->selection.w, canvas->selection.h, TRUE, FALSE);
							//SmudgeCommon( canvas->frame );
							break;
						case MNU_EXTRA_CONTROL:
							//CreateMenuControl( canvas->selection.x, canvas->selection.y, canvas->selection.w, canvas->selection.h, TRUE, FALSE);
							//SmudgeCommon( canvas->frame );
							break;
						}
						//SaveButtonConfig();
					}

}

static void canvasBackgroundMenu( uintptr_t psv, int result ) {
	struct popupParams* params = (struct popupParams*)psv;
	PSI_CONTROL pc = params->pc;
	int32_t px = params->px;
	int32_t py = params->py;
	PSI_CONTROL parent_frame = params->parent_frame;
	PCanvasData canvas = params->canvas;
	params->read = 1;

							if( g.flags.multi_edit )
							{
								PPAGE_DATA page = GetPageFromFrame( pc );
								canvas->current_page = page;
							}
							if( !ProcessPageMenuResult( pc, result ) ) switch( result )
							{
							case MNU_MAKE_CLONE:
								GetCloneButton( canvas, px, py, FALSE );
								break;
							case MNU_EDIT_GLARES:
								EditGlareSets( parent_frame );
								break;
							case MNU_EDIT_FONTS:
								SelectACanvasFont( canvas->pc_canvas, parent_frame, NULL );
								break;
							case MNU_PAGE_PROPERTIES:
								EditCurrentPageProperties(parent_frame,canvas);
								break;
							case MNU_CREATE_PAGE:
								CreateNewPage(parent_frame, canvas);
								break;
							case MNU_RENAME_PAGE:
								RenamePage( canvas->pc_canvas );
								break;
							case MNU_EDIT_DONE:
								AbortConfigureKeys( canvas->pc_canvas, 0 );
								break;
							default:
								Log1( "Unhandled menu option: %ld", result );
								break;
							}

}

static void menuCallback( uintptr_t psv, int result ) {
	struct popupParams* params = (struct popupParams*)psv;
	PSI_CONTROL pc = params->pc;
	int32_t px = params->px;
	int32_t py = params->py;
	PSI_CONTROL parent_frame = params->parent_frame;
	PCanvasData canvas = params->canvas;
	params->read = 1;

#ifdef USE_EDIT_GLARE
					OwnMouse( canvas->edit_glare, TRUE );
#endif

				if( !ProcessPageMenuResult( pc, result ) ) switch( result )
				{
				case MNU_EDIT_BEHAVIORS:
					if( EditControlBehaviors )
					{
						Banner2Message( "EditControlBehaviors has been disabled" );
						/*
							if( canvas->pCurrentControl->flags.bCustom )
							EditControlBehaviors( canvas->pCurrentControl->control );
							else
							EditControlBehaviors( GetKeyCommon( canvas->pCurrentControl->key ) );
							*/
					}
					break;
				case MNU_CLONE:
					InterShell_SetCloneButton( canvas->pCurrentControl );
					break;
				case MNU_COPY:
					InterShell_SetCloneButton( canvas->pCurrentControl );
					InvokeCopyControl( canvas->pCurrentControl );
					break;
				case MNU_PASTE:
					InterShell_SetCloneButton( canvas->pCurrentControl );
					InvokePasteControl( canvas->pCurrentControl );
					break;
				case MNU_EDIT_CONTROL:
					ConfigureKeyEx( pc, canvas->pCurrentControl );
					break;
				case MNU_EDIT_CONTROL_COMMON:
					ConfigureKeyExx( pc, canvas->pCurrentControl, FALSE, TRUE );
					break;
				case MNU_DESTROY_CONTROL:
					// first locate it I suppose...
					DestroyButton( canvas->pCurrentControl );
					canvas->pCurrentControl = NULL;
					break;
				}

}

static int ProcessContextMenu( PCanvasData canvas, PSI_CONTROL pc, int32_t px, int32_t py )
{
			if( canvas->flags.bEditMode )
			{
				canvas->pCurrentControl = MouseInControl( canvas, px, py );
				if( canvas->pCurrentControl && !canvas->flags.bIgnoreKeys )
				{
					PSI_CONTROL parent_frame;
					parent_frame = canvas->edit_glare_frame?canvas->edit_glare_frame:canvas->current_page->frame;
					struct popupParams params_ = { canvas, pc, px, py, parent_frame, 0 };
					static struct popupParams params; params = params_;

					if( g.flags.multi_edit )
					{
						PPAGE_DATA page = GetPageFromFrame( pc );
						canvas->current_page = page;
					}
#ifdef USE_EDIT_GLARE
					TrackPopup_v2( canvas->pControlMenu, parent_frame, menuCallback, (uintptr_t)&params );
					OwnMouse( canvas->edit_glare, TRUE );
#else
					TrackPopup_v2( canvas->pControlMenu, parent_frame, menuCallback, (uintptr_t)&params );
#endif
					//while( !params.read ) Relinquish();
				}
				else if( ( px >= canvas->selection.x && px < ( canvas->selection.x + canvas->selection.w ) )
						  && ( py >= canvas->selection.y && py < ( canvas->selection.y + canvas->selection.h ) )
						  && !canvas->flags.bIgnoreKeys )
				{
					PSI_CONTROL parent_frame;
					parent_frame = canvas->edit_glare_frame?canvas->edit_glare_frame:canvas->current_page->frame;
					struct popupParams params_ = { canvas, pc, px, py, parent_frame, 0 };
					static struct popupParams params; params = params_;
#ifdef USE_EDIT_GLARE
					TrackPopup_v2( g.pSelectionMenu, parent_frame,selectedRegionCallback, (uintptr_t)&params );
#else
					TrackPopup_v2( g.pSelectionMenu, canvas->pc_canvas, selectedRegionCallback, (uintptr_t)&params );
#endif
					//while( !params.read ) Relinquish();
				}
				else
				{
					if( canvas->flags.bSuperMode )
					{
						if( canvas->flags.bEditMode && !canvas->flags.bIgnoreKeys )
						{
							PSI_CONTROL parent_frame;
							parent_frame = canvas->edit_glare_frame?canvas->edit_glare_frame:canvas->current_page->frame;
							struct popupParams params_ = { canvas, pc, px, py, parent_frame, 0 };
							static struct popupParams params; params = params_;
#ifdef USE_EDIT_GLARE
							TrackPopup_v2( canvas->pEditMenu, parent_frame, canvasBackgroundMenu, (uintptr_t)&params );
#else
							TrackPopup_v2( canvas->pEditMenu, canvas->pc_canvas, canvasBackgroundMenu, (uintptr_t)&params );
#endif

						}
						else if( !canvas->flags.bIgnoreKeys )
						{
#if 0
							// someday, finish this bit of code to handle right click
							// (with authentication) to edit configuration.
							uint32_t result = TrackPopup( canvas->pSuperMenu, g.frame );
							if( !ProcessPageMenuResult( pc, result ) ) switch( result )
							{
							case MNU_EDIT_SCREEN:
								canvas->flags.bEditMode = !canvas->flags.bEditMode;
								break;
							default:
								Log1( "Unhandled menu option: %ld", result );
								break;
							}
#endif
						}
					}
				}
			}
			return 0;
}

static void MouseFirstDown( PCanvasData canvas, uintptr_t psv, int32_t px, int32_t py )
{
#ifdef USE_EDIT_GLARE
				OwnMouse( canvas->edit_glare, TRUE );
				SetDisplayNoMouse( canvas->edit_glare, FALSE );
#endif
				// click down left, first time....
				if( canvas->flags.selected && IsControlSelected( canvas, canvas->pCurrentControl ) )
				{
					canvas->flags.sizing_by_corner = DRAG_BEGIN;
#ifndef USE_EDIT_GLARE
					SmudgeCommon( canvas->pc_canvas );
#else
					DrawEditGlare( psv, canvas->edit_glare );
#endif
				}
				else
				{
					PMENU_BUTTON pmc;
					canvas->flags.selected = 0;
					canvas->nSelected = 0;
					canvas->flags.sizing_by_corner = NO_SIZE_OP;

					if( ( pmc = canvas->pCurrentControl ) )
					{
						int tolerance = 1;
						while( ( PARTX(pmc->x+tolerance ) - PARTX( pmc->x ) ) < 20 )
							tolerance++;
						//lprintf( "tolerance is %d	point is %d,%d  c is %d,%d-%d,%d", tolerance, px, py, pmc->x, pmc->y, pmc->w, pmc->h );
						if( ( ( px >= pmc->x ) && ( px < ( pmc->x + tolerance ) ) )
							&& ( ( py >= pmc->y ) && (  py < ( pmc->y + tolerance ) ) ) )
						{
							canvas->flags.sizing_by_corner = UPPER_LEFT;
						}
						else if( ( (px >= pmc->x)  && (px < (pmc->x+tolerance)) )
								  && ( (py <= (pmc->y+pmc->h-1)) && (py > (pmc->y+pmc->h-1-tolerance)) ) )
						{
							canvas->flags.sizing_by_corner = LOWER_LEFT;
						}
						else if( ( (px <= (pmc->x+pmc->w-1)) && (px > (pmc->x+pmc->w-1-tolerance)) )
								  && ( (py >= pmc->y ) && (py < (pmc->y+tolerance) ) ) )
						{
							canvas->flags.sizing_by_corner = UPPER_RIGHT;
						}
						else if( ( (px <= (pmc->x+pmc->w-1)) && (px > (pmc->x+pmc->w-1-tolerance)) )
								  && ( (py <= (pmc->y+pmc->h-1)) && (py > (pmc->y+pmc->h-1-tolerance)) ) )
						{
							canvas->flags.sizing_by_corner = LOWER_RIGHT;
						}
						else
							canvas->flags.sizing_by_corner = DRAG_BEGIN;
#ifndef USE_EDIT_GLARE
						SmudgeCommon( canvas->pc_canvas );
#else
						DrawEditGlare( psv, canvas->edit_glare );
#endif
					}
				}
}

static void MouseDrag( PCanvasData canvas, uintptr_t psv
					  , int32_t px, int32_t py
					  // , pad_left/right, pad top/button (sizing margins)
					  )
{
	/* if first drag */
				if( ( !canvas->flags.dragging
					&& !canvas->flags.sizing )
					)
				{
					PMENU_BUTTON pmc;
					if( ( pmc = canvas->pCurrentControl ) )
					{
						if( canvas->flags.sizing_by_corner )
						{
							if( canvas->flags.sizing_by_corner == DRAG_BEGIN )
							{
								canvas->flags.dragging = 1;
							}
							else
							{
								canvas->flags.sizing = 1;
							}
							g._px = px;
							g._py = py;
#ifndef USE_EDIT_GLARE
							SmudgeCommon( canvas->pc_canvas );
#else
							DrawEditGlare( psv, canvas->edit_glare );
#endif
						}
					}
					else if( !canvas->flags.selecting )
					{
						canvas->flags.selected = 1;
						canvas->flags.selecting = 1;
						canvas->selection._x = canvas->selection.x = px;
						canvas->selection._y = canvas->selection.y = py;
						canvas->selection.w = 1;
						canvas->selection.h = 1;
#ifndef USE_EDIT_GLARE
						//lprintf( "Continue Selecting" );
						SmudgeCommon( canvas->pc_canvas );
#else
						DrawEditGlare( psv, canvas->edit_glare );
#endif
					}
				}

				{
									/* else - subsequent drag */
					if( canvas->flags.selecting )
					{
						int nx, ny, nw, nh;
						// kinda hard to explain why the subtract is -2...
						// but the cell which we start in is always selected,
						// and is one of the corners of the resulting rectangle.
						nx = canvas->selection._x;
						if( ( nw = (px - canvas->selection._x+1) ) <= 0 )
						{
							nx = canvas->selection._x + (nw-1);
							nw = -(nw-2);
						}
						ny = canvas->selection._y;
						if( ( nh = (py - canvas->selection._y+1) ) <= 0 )
						{
							ny = canvas->selection._y + (nh-1);
							nh = -(nh-2);
						}
						canvas->nSelected = SelectItems( canvas, NULL, nx, ny, nw, nh );
						//if( IsSelectionValidEx( canvas, NULL, nx, ny, NULL, NULL, nw, nh ) )
						{
							canvas->selection.x = nx;
							canvas->selection.y = ny;
							canvas->selection.w = nw;
							canvas->selection.h = nh;
						}

						//lprintf( "And now our selection is %d,%d %d,%d" );
#ifndef USE_EDIT_GLARE
						SmudgeCommon( canvas->pc_canvas );
#else
						DrawEditGlare( psv, canvas->edit_glare );
#endif
						//UpdateCommon( pc );
						//SmudgeCommon( pc );
					}
					else if( canvas->flags.sizing || canvas->flags.dragging )
					{
						if( canvas->flags.sizing )
						{
							int dx = px - g._px
								, dy = py - g._py;
							//if( IsSelectionValidEx( canvas->frame
							//							  , canvas->pCurrentControl
							//							 , canvas->pCurrentControl->x
							//							 , canvas->pCurrentControl->y
							//  						 , &dx, &dy
							//							 , canvas->pCurrentControl->w
							//							 , canvas->pCurrentControl->h ) )
							{
retry:
								switch( canvas->flags.sizing_by_corner )
								{
								case UPPER_LEFT:
									if( ( (int)canvas->pCurrentControl->w - (int)dx ) <= 0 )
									{
										canvas->flags.sizing_by_corner = UPPER_RIGHT;
										goto retry;
									}
									if( ( (int)canvas->pCurrentControl->h - (int)dy ) <= 0 )
									{
										canvas->flags.sizing_by_corner = LOWER_LEFT;
										goto retry;
									}
									canvas->pCurrentControl->x += dx;
									canvas->pCurrentControl->w -= dx;
									canvas->pCurrentControl->y += dy;
									canvas->pCurrentControl->h -= dy;

									break;
								case UPPER_RIGHT:
									if( ( (int)canvas->pCurrentControl->w + (int)dx ) <= 0 )
									{
										canvas->flags.sizing_by_corner = UPPER_LEFT;
										goto retry;
									}
									if( ( (int)canvas->pCurrentControl->h - (int)dy ) <= 0 )
									{
										canvas->flags.sizing_by_corner = LOWER_RIGHT;
										goto retry;
									}
									canvas->pCurrentControl->w += dx;
									canvas->pCurrentControl->y += dy;
									canvas->pCurrentControl->h -= dy;
									break;
								case LOWER_LEFT:
									if( ( (int)canvas->pCurrentControl->w - (int)dx ) <= 0 )
									{
										canvas->flags.sizing_by_corner = LOWER_RIGHT;
										goto retry;
									}
									if( ( (int)canvas->pCurrentControl->h + (int)dy ) <= 0 )
									{
										canvas->flags.sizing_by_corner = UPPER_LEFT;
										goto retry;
									}
									canvas->pCurrentControl->x += dx;
									canvas->pCurrentControl->w -= dx;
									canvas->pCurrentControl->h += dy;
									break;
								case LOWER_RIGHT:
									if( ( (int)canvas->pCurrentControl->w + (int)dx ) <= 0 )
									{
										canvas->flags.sizing_by_corner = LOWER_LEFT;
										goto retry;
									}
									if( ( (int)canvas->pCurrentControl->h + (int)dy ) <= 0 )
									{
										canvas->flags.sizing_by_corner = UPPER_RIGHT;
										goto retry;
									}
									canvas->pCurrentControl->w += dx;
									canvas->pCurrentControl->h += dy;
									break;
								}
								canvas->pCurrentControl->flags.bMoved = 1;
								g._px += dx;
								g._py += dy;
							}
						}
						else if( canvas->flags.dragging )
						{
							if( canvas->flags.selected )
							{
								int dx = px - g._px;
								int dy = py - g._py;
								if( ( dx || dy ) )
								{
									INDEX idx;
									PMENU_BUTTON item;
									LIST_FORALL( canvas->selected_list, idx, PMENU_BUTTON, item )
									{
										item->x += dx;
										item->y += dy;
										MoveCommon( QueryGetControl( item )
											, PARTX( item->x )
											, PARTY( item->y )
											);
									}
									g._px += dx;
									g._py += dy;
									canvas->selection.x += dx;
									canvas->selection.y += dy;
								}
#ifndef USE_EDIT_GLARE
								SmudgeCommon( canvas->pc_canvas );
#else
								DrawEditGlare( psv, canvas->edit_glare );
#endif
							}
							else
							{
								int dx = px - g._px
									, dy = py - g._py;
								if( ( dx || dy ) && canvas->pCurrentControl )
								{
									if( IsSelectionValidEx( canvas
										, canvas->pCurrentControl
										, (int32_t)canvas->pCurrentControl->x
										, (int32_t)canvas->pCurrentControl->y
										, &dx, &dy
										, (uint32_t)canvas->pCurrentControl->w
										, (uint32_t)canvas->pCurrentControl->h ) )
									{
										canvas->pCurrentControl->x += dx;
										canvas->pCurrentControl->y += dy;
										g._px += dx;
										g._py += dy;
										canvas->pCurrentControl->flags.bMoved = 1;
									}
								}
							}
						}

						if( canvas->pCurrentControl->flags.bMoved )
						{
							canvas->pCurrentControl->flags.bMoved = 0;
							MoveSizeCommon( QueryGetControl( canvas->pCurrentControl )
								, PARTX( canvas->pCurrentControl->x )
								, PARTY( canvas->pCurrentControl->y )
								, PARTW( canvas->pCurrentControl->x, canvas->pCurrentControl->w )
								, PARTH( canvas->pCurrentControl->y, canvas->pCurrentControl->h )
								);
#ifndef USE_EDIT_GLARE
							SmudgeCommon( canvas->pc_canvas );
#else
							DrawEditGlare( psv, canvas->edit_glare );
#endif
						}
					}
				}
}

static void MouseFirstRelease( PCanvasData canvas, uintptr_t psv, int32_t px, int32_t py, uint32_t b )
{



				// first release after having been down until now.
				if( canvas->flags.sizing ||( canvas->flags.selecting )||( canvas->flags.dragging ))
				{
					canvas->flags.sizing_by_corner = NO_SIZE_OP;
					canvas->flags.selecting = 0;
					canvas->flags.sizing = 0;
					canvas->flags.dragging = 0;
#ifndef USE_EDIT_GLARE
					SmudgeCommon( canvas->pc_canvas );
#else
					DrawEditGlare( psv, canvas->edit_glare );
#endif
				}
				else
				{
				// first release after having been down until now.
					if( canvas->flags.sizing_by_corner)
					{
						canvas->flags.sizing_by_corner = NO_SIZE_OP;
#ifndef USE_EDIT_GLARE
						SmudgeCommon( canvas->pc_canvas );
#else
						DrawEditGlare( psv, canvas->edit_glare );
#endif
					}
					if( canvas->pCurrentControl && canvas->flags.bEditMode )
					{
						// got put in a current control.
						// and edit mode.
						// did not begin selecting, sizeing or draging
						// pop up current configuration?
						if( b & MK_CONTROL )
						{
							INDEX idx;
							PMENU_BUTTON item;
							LIST_FORALL( canvas->selected_list, idx, PMENU_BUTTON, item )
							{
								if( canvas->pCurrentControl == item )
								{
									DeleteLink( &canvas->selected_list, item );
									canvas->nSelected--;
									if( !canvas->nSelected )
									{
										if( canvas->flags.selected )
										{
											canvas->flags.selected = 0;
											// maybe we need position?
										}
									}
									break;
								}
							}
							if( !item )
							{
								AddLink( &canvas->selected_list, canvas->pCurrentControl );
								canvas->nSelected++;
								if( !canvas->flags.selected )
								{
									canvas->flags.selected = 1;
								}
							}
#ifndef USE_EDIT_GLARE
							SmudgeCommon( canvas->pc_canvas );
#else
							DrawEditGlare( psv, canvas->edit_glare );
#endif
						}
						else
						{
							if( canvas->pCurrentControl && canvas->flags.bEditMode )
							{
								// if not a custom control it's a fancy key-button
								//if( !canvas->pCurrentControl->flags.bCustom )
								//	ReleaseCommonUse( GetKeyCommon( canvas->pCurrentControl->key ) );
								ConfigureKeyEx( canvas->pc_canvas, canvas->pCurrentControl );
							}
						}
					}
					else if( canvas->flags.bEditMode )
					{
						// I dunno this seemed like a good idea...
						//ProcessContextMenu( canvas, canvas->pc_canvas, px, py );
						EmptyList( &canvas->selected_list );
						canvas->nSelected = 0;
						canvas->flags.selected = 0;
#ifndef USE_EDIT_GLARE
						SmudgeCommon( canvas->pc_canvas );
#else
						DrawEditGlare( psv, canvas->edit_glare );
#endif
					}
				}
#ifdef USE_EDIT_GLARE
				OwnMouse( canvas->edit_glare, FALSE );
				SetDisplayNoMouse( canvas->edit_glare, FALSE );
#endif
}

int CPROC MouseEditGlare( uintptr_t psv, int32_t x, int32_t y, uint32_t b )
{
	PCanvasData canvas = (PCanvasData)psv;
	PSI_CONTROL pc = canvas->pc_canvas; // was the canvas mouse routine so...
	static uint32_t _b;
	static int32_t _x, _y;
	static int _px, _py;
	int px, py;
	//lprintf( "Glare mouse %d %d %d", x, y, b );
#define PARTOFX(xc) ( ( xc ) * canvas->current_page->grid.nPartsX ) / canvas->width
#define PARTOFY(yc) ( ( yc ) * canvas->current_page->grid.nPartsY ) / canvas->height
	px = PARTOFX( x );
	py = PARTOFY( y );

	// shell mouse is frame mouse?
	//lprintf( "Shell mouse %d,%d %d", x, y,  b );

	// current control will only be picked while mouse button up...
	if( !(b & MK_LBUTTON) )
	{
		canvas->pCurrentControl = MouseInControl( canvas, px, py );
	}

	do
	{
		if( canvas->flags.bEditMode )
		{
			if( ( b == _b ) && ( canvas->flags.dragging || canvas->flags.sizing || canvas->flags.selecting ) )
				if( ( _px == px ) && ( _py == py ) )
					break;
			//lprintf( "Mouse %d,%d %d,%d %08x", px, py, x, y, b );
			if( b & MK_LBUTTON)
			{
				//lprintf( "left down." );
				if( !(_b & MK_LBUTTON ) )
				{
					MouseFirstDown( canvas, psv, px, py );
				}
				else
				{
					// was down, is down....
					MouseDrag( canvas, psv, px, py );
				}
			}
			else
			{
				if( _b & MK_LBUTTON )
				{
					MouseFirstRelease( canvas, psv, px, py, b );
				}
				else
				{
					// was released, still released.
				}
			}
		}
	}
	while( 0 );

	//-----------------------------------------------------
	///	Right button context menu stuff
	//-----------------------------------------------------
	//Log5( "Mouse event: %d,%d %x %x %x", x, y, b, _b, MK_RBUTTON );
	if( b & MK_RBUTTON )
	{
		if( !(_b & MK_RBUTTON) )
		{
			if( ProcessContextMenu( canvas, pc, px, py ) )
				b &= ~MK_RBUTTON;
		}
	}

	_px = px;
	_py = py;
	_b = b;
	_x = x;
	_y = y;
	return 1;
}



//---------------------------------------------------------------------------

void CPROC QuitMenu( PSI_CONTROL pc, uint32_t keycodeUnused )
{
	//ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );
	lprintf( "!!!!!!!!!!! QUIT MENU !!!!!!!!!!!!!!!" );
	g.flags.bExit = 1;
	Banner2TopNoWait( "Shutting down..." );
	InvokeInterShellShutdown();
	{
		PSI_CONTROL canvas;
		INDEX idx;
		LIST_FORALL( g.frames, idx, PSI_CONTROL, canvas )
		{
			EnableFrameUpdates( canvas, FALSE );
		}
		//PMENU_BUTTON button;
		//INDEX idx;
		if( g.flags.multi_edit )
		{
			//PPAGE_DATA page;
			//INDEX idx;
			/*
			LIST_FORALL( g.pages, idx, PPAGE_DATA, page )
			{
			EnableFrameUpdates( page->frame, FALSE );
			}
			*/
		}
		//else
		//	EnableFrameUpdates( g.single_frame, FALSE );
		//LIST_FORALL( canvas->current_page->controls, idx, PMENU_BUTTON, button )
		//{
		//	lprintf( "Destroy button..." );
		//	DestroyButton( button );
		//}
		// destroy frame here (in keyboard handler)
		// ends up hanging forever waiting for the destruction of self
		//DestroyFrame( &g.frame );
	}
	lprintf( "Waking thread..." );
	WakeThread( g.pMainThread );
}

//---------------------------------------------------------------------------

void CPROC RestartMenu( uintptr_t psv, uint32_t keycode )
{
	g.flags.bExit = 2;
	WakeThread( g.pMainThread );
}

//---------------------------------------------------------------------------

void CPROC ResumeMenu( uintptr_t psv, uint32_t keycode )
{
	g.flags.bExit = 3;
	WakeThread( g.pMainThread );
}

//---------------------------------------------------------------------------

void EndEditingPage( PSI_CONTROL pc_canvas, PPAGE_DATA page )
{
	PCanvasData canvas = GetCanvas( pc_canvas );
	EnableFrameUpdates( page->frame, FALSE );
	RestorePage( canvas, canvas->current_page, TRUE );
	EnableFrameUpdates( page->frame, TRUE );
	SmudgeCommon( page->frame );
}

void CPROC AbortConfigureKeys( PSI_CONTROL pc, uint32_t keycode )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );
	if( canvas->flags.bEditMode )
	{
		//lprintf( "Disallow frame updates now.." );
		canvas->flags.bEditMode = FALSE;

#ifdef USE_EDIT_GLARE
		HideDisplay( canvas->edit_glare );
#endif

		canvas->flags.selected = 0;
		canvas->nSelected = 0;
		SaveButtonConfig( pc, g.config_filename );
		EndEditingPage( pc, canvas->current_page );
		//lprintf( "And having enabled them... then..." );
	}
	InvokeEndEditMode();
	if( g.psv_edit_security )
	{
		CloseSecurityContext( (uintptr_t)pc, (uintptr_t)g.psv_edit_security );
	}
}

LOGICAL CPROC EventAbortConfigureKeys( uintptr_t psv, uint32_t keycode )
{
	AbortConfigureKeys( (PSI_CONTROL)psv, keycode );
	return 1;
}

//---------------------------------------------------------------------------

void BeginEditingPage( PPAGE_DATA page )
{
	PMENU_BUTTON button;
	INDEX idx;
	InvokeBeginEditMode();
	LIST_FORALL( page->controls, idx, PMENU_BUTTON, button )
	{
		HideControl( QueryGetControl( button ) );
		InvokeEditBegin( button );
	}
}

void CPROC ConfigureKeys( PSI_CONTROL pc, uint32_t keycode )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );
	if( !canvas )
	{
		return;
	}
	{
		g.psv_edit_security = CreateSecurityContext( (uintptr_t)pc );
		if( g.psv_edit_security != INVALID_INDEX )
		{
			if( g.psv_edit_security )
			{
				//Log Password Action, enable configuration
			}
		}
		else
			return;
	}
	// hide everything, turn on edit mode
	// which draws fake controls, so that
	// we can provide appropriate configuration
	// menus.
	canvas->flags.bEditMode = TRUE;
#ifdef USE_EDIT_GLARE

	lprintf( "Restore glare..." );
	RestoreDisplay( canvas->edit_glare );
	lprintf( "Glare restored..." );
	MakeTopmost( canvas->edit_glare );
#else
	EnableFrameUpdates( pc, FALSE );
	BeginEditingPage( canvas->current_page );
	EnableFrameUpdates( pc, TRUE );
	SmudgeCommon( pc );
#endif

}

void CPROC Exit( uintptr_t psv, PKEY_BUTTON key )
{
	exit(0);
}

void CPROC CheckMemStats( uintptr_t psv )
{
	static uint32_t _a, _b, _c, _d;
	uint32_t a, b,c, d;
#define f(a) ((_##a)==(a))
#define h(a) ((_##a)=(a))
	GetMemStats( &a, &b, &c, &d );
	if( f(a)||f(b)||f(c)||f(d))
	{
		h(a),h(b),h(c),h(d);
		lprintf( "---***--- MemStats  free: %" _32f " used: %" _32f " used chunks: %" _32f " free chunks: %" _32f, a, b, c-d, d );
		if( _c != c )
		{
			_c = c;
			//DebugDumpMem();
		}
	}
#undef f
#undef h
}

INTERSHELL_PROC( void, SetButtonTextField )( PMENU_BUTTON pKey, PTEXT_PLACEMENT pField, TEXTCHAR *text )
{
	SetKeyTextField( pField, text );
}
INTERSHELL_PROC( PTEXT_PLACEMENT, AddButtonLayout )( PMENU_BUTTON pKey, int x, int y, SFTFont *font, CDATA color, uint32_t flags )
{
	return AddKeyLayout( pKey->control.key, x, y, font, color, flags );
}


void GetPageSizeEx( PSI_CONTROL pc_canvas, uint32_t* width, uint32_t* height )
{
	if( !pc_canvas )
	{
		if( width )
			(*width) = 0;
		if( height )
			(*height) = 0;
	}
	else
	{
		ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
		if( canvas )
		{
			GetFrameSize( pc_canvas, width, height );
		}
	}
}

void GetPageSize( uint32_t* width, uint32_t* height )
{
	//return
	GetPageSizeEx( NULL, width, height );
}

void ProbeDisplaySize( void )
{
	if( g.flags.bUseCustomPositioning )
	{
		// page sizes will already be set; there will be no change for this.
	}
	else if( g.target_display > 0 )
	{
		GetDisplaySizeEx( g.target_display, &g.default_page_x, &g.default_page_y, &g.default_page_width, &g.default_page_height );
	}
	else
	{
		g.default_page_x = 0;
		g.default_page_y = 0;
		GetDisplaySizeEx( 0, NULL, NULL, &g.default_page_width, &g.default_page_height );
		if( g.flags.multi_edit )
		{
			g.default_page_width = g.default_page_width * 3 / 8;
			g.default_page_height = g.default_page_height * 3 / 8;
		}
		if( g.flags.bSpanDisplay )
		{
			g.default_page_width = g.default_page_width*2;
			g.default_page_height = g.default_page_height;
		}
	}
}

PSI_CONTROL SetupSystemsListAndGlobalSingleFrame(void )
{
	PSI_CONTROL result_canvas = NULL;
	//SystemLogTime( SYSLOG_TIME_DELTA|SYSLOG_TIME_CPU );
	//AddTimer( 15000, CheckMemStats, 0 );
	//Log( "Menu started\n" );
	uint32_t width, height;
	if( !g.systems && g.flags.bSQLConfig )
	{
#if 0
		CTEXTSTR *result;
		// get systems from SQL list right now
		// should be a small count...
		// maybe I don't want this sort of thing here at all?
#ifndef __NO_SQL__
		if( DoSQLRecordQuery( "select count(*) from systems", NULL, &result, NULL ) && result )
		{
			int n;
			g.systems = (struct system_info*)Allocate( sizeof( *g.systems ) * ( atoi( result[0] ) + 1 ) );
			g.systems[0].me = &g.systems;
			g.systems[0].name = SegCreateFromText( g.system_name );
			g.systems[0].ID = INVALID_INDEX;
			g.systems[0].next = NULL;

			for( n = 1, DoSQLRecordQuery( "select name from systems", NULL, &result, NULL );
				result;
				n++, GetSQLRecord( &result ) )
			{
				g.systems[n].name = SegCreateFromText( result[0] );
				g.systems[n].ID = INVALID_INDEX;
				if( n )
				{
					/* don't release this... will have to rework what this is... */
					g.systems[n].me = &g.systems[n-1].next;
					g.systems[n].next = NULL;
					g.systems[n-1].next = g.systems + n;
				}
			}
		}
		else
#endif
#endif
		{
			//int n;
			g.systems = (struct system_info*)Allocate( sizeof( *g.systems ) * ( 1 ) );
			g.systems[0].me = &g.systems;
			g.systems[0].name = SegCreateFromText( g.system_name );
			g.systems[0].ID = INVALID_INDEX;
			g.systems[0].next = NULL;
		}
	}
#ifdef USE_INTERFACES
	if( !SetControlInterface( g.pRenderInterface = GetDisplayInterface() ) )
	{
		Log( "Failed to set display interface..." );
		return NULL;
	}
	if( !SetControlImageInterface( g.pImageInterface = GetImageInterface() ) )
	{
		Log( "Failed to set image interface..." );
		DropDisplayInterface( g.pRenderInterface );
		return NULL;
	}
#endif
#ifndef __NO_OPTIONS__
	if( !g.flags.multi_edit )
		g.flags.multi_edit = SACK_GetProfileIntEx( GetProgramName(), "Intershell Layout/Windowed mode (not full screen)", 0, TRUE );
	g.flags.bLogKeypresses = SACK_GetProfileIntEx( GetProgramName(), "Intershell/Log button events", 0, TRUE );
#endif
	SetBlotMethod( BLOT_C );
	GetDisplaySizeEx( 0, NULL, NULL, &width, &height );
	if( g.flags.multi_edit )
	{
		width = width * 3 / 8;
		height = height * 3 / 8;
	}

	if( !g.flags.bExternalApplicationhost )
	{
#ifndef __NO_OPTIONS__
		g.flags.bSpanDisplay = SACK_GetProfileIntEx( GetProgramName(), "Intershell Layout/Use Both Displays(horizontal)", 0, TRUE );
		if( g.flags.bSpanDisplay )
		{
			width = width*2;
			height = height;
			lprintf( "opening canvas at %d,%d %dx%d", 0, 0, width, height );
			result_canvas = MakeControl( NULL, menu_surface.TypeID
												 , 0, 0
												 , width, height, 0 );
		}
#  ifndef __LINUX__
		if( !result_canvas )
		{
			g.target_display = SACK_GetProfileIntEx( GetProgramName(), "Intershell Layout/Use Screen Number", 0, TRUE );
			if( g.target_display > 0 )
			{
				uint32_t w, h;
				int32_t x, y;
				GetDisplaySizeEx( g.target_display, &x, &y, &w, &h );
				lprintf( "opening canvas on screen %d at %d,%d %dx%d", g.target_display, x, y, w, h );
				result_canvas = MakeControl( NULL, menu_surface.TypeID
													 , x, y
													 , w, h, 0 );
			}
		}
#  endif
		if( !result_canvas )
		{
			g.flags.bUseCustomPositioning = SACK_GetProfileIntEx( GetProgramName(), "Intershell Layout/Use Custom Positioning", 0, TRUE );
			if( g.flags.bUseCustomPositioning )
			{
				g.default_page_x = SACK_GetProfileInt( GetProgramName(), "Intershell Layout/X Position", 0 );
				g.default_page_y = SACK_GetProfileInt( GetProgramName(), "Intershell Layout/Y Position", 0 );
				g.default_page_width = SACK_GetProfileInt( GetProgramName(), "Intershell Layout/Width", width );
				g.default_page_height = SACK_GetProfileInt( GetProgramName(), "Intershell Layout/Height", height );
				//lprintf( "opening canvas at %d,%d %dx%d", g.default_page_x, g.default_page_y, g.default_page_width, g.default_page_height );
				result_canvas = MakeControl( NULL, menu_surface.TypeID, g.default_page_x, g.default_page_y, g.default_page_width, g.default_page_height, 0 );
			}
			else
#endif
			{
				//lprintf( "opening canvas at 0,0, %dx%d %d", width, height, menu_surface.TypeID );
				result_canvas = MakeControl( NULL, menu_surface.TypeID, 0, 0, width, height, 0 );
			}
			SetControlText( result_canvas, g.single_frame_title );
#ifndef __NO_OPTIONS__
		}
#endif
		// always have at least 1 frame.  and it is a menu canvas
		InterShell_DisablePageUpdate( result_canvas, TRUE );
		GetFrameSize( result_canvas, &g.default_page_width, &g.default_page_height );
		GetFramePosition( result_canvas, &g.default_page_x, &g.default_page_y );
		//GetDisplayPosition( GetFrameRenderer( result_canvas ), &g.default_page_x, &g.default_page_y, &g.default_page_width, &g.default_page_height );
		//lprintf( "Got single frame. %d,%d", g.width, g.height );
		UseACanvasFont( result_canvas, "Default" );
	}
	//lprintf( "Got single frame. %d,%d", g.width, g.height );
	return result_canvas;
}


//---------------------------------------------------------------------------
static INDEX iFocus = INVALID_INDEX;
void CPROC DoKeyRight( PSI_CONTROL pc, uint32_t key )
{
	PMENU_BUTTON button;
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );
	iFocus++;
	button = (PMENU_BUTTON)GetLink( &canvas->current_page->controls, iFocus );
	// should query (if can focus)
	if( button )
		SetCommonFocus( QueryGetControl( button ) );
}

void CPROC DoKeyLeft( PSI_CONTROL pc, uint32_t key )
{
	PMENU_BUTTON button;
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );
	if( iFocus )
		iFocus--;
	button = (PMENU_BUTTON)GetLink( &canvas->current_page->controls, iFocus );
	// should query (if can focus)
	if( button )
		SetCommonFocus( QueryGetControl( button ) );
}

void CPROC DoKeyUp( PSI_CONTROL pc, uint32_t key )
{
	PMENU_BUTTON button;
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );
	if( iFocus )
		iFocus--;
	button = (PMENU_BUTTON)GetLink( &canvas->current_page->controls, iFocus );
	// should query (if can focus)
	if( button )
		SetCommonFocus( QueryGetControl( button ) );
}

void CPROC DoKeyDown( PSI_CONTROL pc, uint32_t key )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );
	PMENU_BUTTON button;
	iFocus++;
	button = (PMENU_BUTTON)GetLink( &canvas->current_page->controls, iFocus );
	// should query (if can focus)
	if( button )
		SetCommonFocus( QueryGetControl( button ) );
}

//---------------------------------------------------------------------------
static PMENU MakeControlsMenu( PMENU parent, TEXTCHAR const *basename, CTEXTSTR priorname )
{
	static int n = 0;
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	PMENU pExtraCreate = NULL;
	for( name = GetFirstRegisteredName( basename, &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		PMENU submenu;
		if( priorname &&
			( ( strcmp( name, "button_create" ) == 0 ) ||
			( strcmp( name, "control_create" ) == 0 ) ||
			( strcmp( name, "listbox_create" ) == 0 ) ) )
		{
			// okay then add this one...
			//snprintf( newname, sizeof( newname ), "%s/%s", basename, name );
			//if( NameHasBranches( &data ) )
			{
				// eat the first two parts - intershell/controls/
				// create the control name as that...
				TEXTCHAR *controlpath = strchr( basename, '/' );
				if( controlpath )
				{
					controlpath++;
					controlpath = strchr( controlpath, '/' );
					if( controlpath )
						controlpath++;
				}

				AddLink( &g.extra_types, StrDup( controlpath ) );

				AppendPopupItem( parent, MF_STRING
					, MNU_CREATE_EXTRA + n
					, priorname );
				n++;
				break;
			}
		}
		else
		{
			if( NameHasBranches( &data ) )
			{
				TEXTCHAR newname[256];
				if( !pExtraCreate )
					pExtraCreate = CreatePopup();
				snprintf( newname, sizeof( newname ), "%s/%s", basename, name );
				submenu = MakeControlsMenu( pExtraCreate, newname, name );
				if( submenu )
					AppendPopupItem( pExtraCreate, MF_STRING|MF_POPUP
					, (uintptr_t)submenu
					, name );
			}
		}
	}
	return pExtraCreate;
}

//---------------------------------------------------------------------------

int CommonInitCanvas( PSI_CONTROL pc_canvas, PCanvasData canvas )
{
#if defined( ALLOW_GLOBAL_FRAME )
	if( !g.first_frame )
	{
		// this is the first canvas, therefore we assume it's the default.
		//  a lot of code has moved away from g.single_frame references; but there still remains
		// places that the global frame is used.
		//lprintf( "Half way through init, and now we set single_frame, could not wait for return to creator." );
		g.first_frame = pc_canvas;
	}
#endif

	g.flags.bExit = 0;

	if( !canvas->pPageMenu )
	{
		canvas->pPageMenu = CreatePopup();
		AppendPopupItem( canvas->pPageMenu, MF_STRING|MF_CHECKED
			, MNU_CHANGE_PAGE + ( canvas->default_page->ID = 0 )
			, "First Page" );
	}
	if( !canvas->pPageUndeleteMenu )
	{
		canvas->pPageUndeleteMenu = CreatePopup();
	}
	if( !canvas->pPageDestroyMenu )
	{
		canvas->pPageDestroyMenu = CreatePopup();
	}
	if( !g.pSelectionMenu )
	{
		g.pSelectionMenu = CreatePopup();
	}
	if( !canvas->pControlMenu )
	{
		canvas->pControlMenu = CreatePopup();
		AppendPopupItem( canvas->pControlMenu, MF_STRING, MNU_EDIT_CONTROL, "Edit" );
		AppendPopupItem( canvas->pControlMenu, MF_STRING, MNU_EDIT_CONTROL_COMMON, "Edit General" );
		AppendPopupItem( canvas->pControlMenu, MF_STRING, MNU_CLONE, "Clone" );
		if( EditControlBehaviors )
			AppendPopupItem( canvas->pControlMenu, MF_STRING, MNU_EDIT_BEHAVIORS, "Edit Behaviors" );
		AppendPopupItem( canvas->pControlMenu, MF_STRING, MNU_DESTROY_CONTROL, "Destroy" );
	}

	if( !canvas->pSuperMenu )
	{
		canvas->pSuperMenu = CreatePopup();
		AppendPopupItem( canvas->pSuperMenu, MF_STRING|MF_UNCHECKED, MNU_EDIT_SCREEN, "Edit Screen" );
	}

	if( !canvas->pEditMenu )
	{
		canvas->pEditMenu = CreatePopup();
		AppendPopupItem( canvas->pEditMenu, MF_STRING, MNU_PAGE_PROPERTIES, "Properties" );
		if( !g.pGlobalPropertyMenu )
			g.pGlobalPropertyMenu = CreatePopup();
		AppendPopupItem( canvas->pEditMenu, MF_STRING|MF_POPUP, (uintptr_t)g.pGlobalPropertyMenu, "Other Properties..." );
		AppendPopupItem( canvas->pEditMenu, MF_STRING, MNU_EDIT_FONTS, "Edit Fonts" );
		AppendPopupItem( canvas->pEditMenu, MF_STRING, MNU_EDIT_GLARES, "Edit Button Glares" );
		AppendPopupItem( canvas->pEditMenu, MF_STRING, MNU_CREATE_PAGE, "Create Page" );
		AppendPopupItem( canvas->pEditMenu, MF_STRING, MNU_RENAME_PAGE, "Rename Page" );
		AppendPopupItem( canvas->pEditMenu, MF_STRING, MNU_MAKE_CLONE, "Create Clone" );
		AppendPopupItem( canvas->pEditMenu, MF_STRING|MF_POPUP, (uintptr_t)canvas->pPageMenu, "Change Page" );
		AppendPopupItem( canvas->pEditMenu, MF_STRING|MF_POPUP, (uintptr_t)canvas->pPageDestroyMenu, "Destroy Page" );
		AppendPopupItem( canvas->pEditMenu, MF_STRING|MF_POPUP, (uintptr_t)canvas->pPageUndeleteMenu, "Undestroy Page" );
		AppendPopupItem( canvas->pEditMenu, MF_STRING, MNU_EDIT_DONE, "Done" );
	}

	return 0;
}

static void OnHideCommon( "menu canvas" )( PSI_CONTROL pc )
{
	//lprintf( "A control's hide has been invoked and that control is a cavnas... hide controls on my page." );
	HidePageEx( pc );
}

static void OnRevealCommon( "menu canvas" )( PSI_CONTROL pc )
{
	if( !g.flags.multi_edit )
	{
		//lprintf( "Restoring page..." );
		RestoreCurrentPage( pc );
		//lprintf( "restored page..." );
	}
}

static LOGICAL CPROC AcceptFiles( PSI_CONTROL pc, CTEXTSTR file, int32_t x, int32_t y )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );

	static int bInvoked;
	int px, py;
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	if( bInvoked )
		return FALSE;
	bInvoked = TRUE;

//#define PARTOFX(xc) ( ( xc ) * canvas->current_page->grid.nPartsX ) / canvas->width
//#define PARTOFY(yc) ( ( yc ) * canvas->current_page->grid.nPartsY ) / canvas->height
	px = PARTOFX( x );
	py = PARTOFY( y );

	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/Drop Accept", &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		LOGICAL (CPROC *f)(PSI_CONTROL,CTEXTSTR, int,int);
		//snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/common/save common/%s", name );
		f = GetRegisteredProcedure2( (CTEXTSTR)data, LOGICAL, name, (PSI_CONTROL,CTEXTSTR, int,int) );
		if( f )
			if( f(pc, file,px,py) )
				break;
	}
	bInvoked = FALSE;
	return TRUE;
}

static int OnCreateCommon( "Menu Canvas" )( PSI_CONTROL pc )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );
	SetCommonTransparent( pc, FALSE );
	{
		//PSI_CONTROL parent;
		//parent = GetFrame( pc );
		//if( parent )
		{
			int displays_wide =
#ifndef __NO_OPTIONS__
				SACK_GetProfileIntEx( GetProgramName(), "Intershell Layout/Expected displays wide", 1, TRUE );
#else
				1;
#endif
			int display_native_width =
#ifndef __NO_OPTIONS__
				SACK_GetProfileIntEx( GetProgramName(), "Intershell Layout/Native display width", 1024, TRUE );
#else
				1024;
#endif
			int displays_high =
#ifndef __NO_OPTIONS__
				SACK_GetProfileIntEx( GetProgramName(), "Intershell Layout/Expected displays high", 1, TRUE );
#else
				1;
#endif
			int display_native_height =
#ifndef __NO_OPTIONS__
				SACK_GetProfileIntEx( GetProgramName(), "Intershell Layout/Native Display Height", 768, TRUE );
#else
				768;
#endif
			Image surface = GetControlSurface( pc );
			canvas->pc_canvas = pc; // self reference
			canvas->width = surface->width;
			canvas->height = surface->height;

			canvas->current_page = (PPAGE_DATA)Allocate( sizeof( *canvas->current_page ) );
			MemSet( canvas->current_page, 0, sizeof( *canvas->current_page ) );
			AddLink( &canvas->pages, canvas->current_page );
			canvas->default_page =  canvas->current_page;
			canvas->default_page->frame = pc;
			canvas->current_page->flags.bActive = 1;
			canvas->width_scale.denominator = displays_wide * display_native_width;
			canvas->width_scale.numerator =  surface->width;
			canvas->height_scale.denominator = displays_high * display_native_height;
			canvas->height_scale.numerator = surface->height;
			// current page is set to default page here (usually)
			// this sets the initial page with no config to 40x40 squares
			canvas->current_page->grid.nPartsX = displays_wide * 40;
			canvas->current_page->grid.nPartsY = displays_high * 40;
			canvas->current_page->frame = canvas->pc_canvas;
			// really this should have been done the other way...
			canvas->prior_pages = CreateLinkStackLimited( 20 );

			canvas->flags.bSuperMode = 1;
			AddLink( &g.frames, pc );
			CommonInitCanvas( pc, canvas );
		}
		AddCommonAcceptDroppedFiles( pc, AcceptFiles );
		//else
		//	canvas->current_page = parent->current_page;
	}
	return 1;
}

int CPROC PageFocusChanged( PSI_CONTROL pc, LOGICAL bFocused )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );
	if( g.flags.multi_edit )
	{
		if( bFocused )
		{
			canvas->current_page = GetPageFromFrame( pc );
			if( canvas->current_page )
				lprintf( "*** New current page: %s", canvas->current_page->title?canvas->current_page->title:"[StartupPage]" );
			else
				lprintf( "*** FAULT not a frame of a page..." );
			//g.single_frame = pc;
		}
		//canvas->frame = pc;
	}
	return 1;
}

LOGICAL CPROC GoodQuitMenu( uintptr_t psvUnused, uint32_t keycodeUnused )
{
	Banner2NoWaitAlpha( "Exiting..." );
	if( g.flags.bTerminateStayResident )
	{
		BAG_Exit(0xd1e);
	}
	else
	{
		g.flags.bExit = 1;
		WakeThread( g.pMainThread );
	}
	return TRUE;
}

LOGICAL CPROC DoConfigureKeys( uintptr_t psv, uint32_t keycodeUnused )
{
	PCanvasData canvas = (PCanvasData)psv;
	if( !g.flags.bNoEdit )
		ConfigureKeys( canvas->pc_canvas, keycodeUnused );
	return TRUE;
}

static int OnKeyCommon( "Menu Canvas" )( PSI_CONTROL pc, uint32_t key )
{
	lprintf( "Key input: %08x %x isalt?%x alt:%x sft:%x", key, KEY_C, key &KEY_ALT_DOWN, KEY_ALT_DOWN, KEY_SHIFT_DOWN);
	if(( (key & (KEY_ALT_DOWN|KEY_SHIFT_DOWN))==(KEY_ALT_DOWN|KEY_SHIFT_DOWN) )
		||( (key & (KEY_ALT_DOWN))==(KEY_ALT_DOWN) )
		)
	{
		lprintf( "is shift-alt and key? %d %d", KEY_CODE(key), KEY_C, g.flags.bNoEdit );
		if( !g.flags.bNoEdit )
		{
			if( ( KEY_CODE( key ) == KEY_C ) )
			{
				lprintf( "-------- CONFIGURE KEYS --------------");
				ConfigureKeys( pc, key );
				return TRUE;
			}
		}
	}
	if( KEY_CODE( key ) == KEY_ESCAPE )
	{
		ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc );
		if( canvas && canvas->flags.bEditMode )
		{
			AbortConfigureKeys( pc, key );
			return TRUE;
		}
		return FALSE;
	}
	return 0;
}

#ifndef NO_TOUCH
static struct {
	int prior_x, prior_y;
} touch_state;

static void HandleSingleTouch( PCanvasData canvas, PINPUT_POINT touch1, PINPUT_POINT touch2 )
{
	if( touch1->flags.new_event || touch2->flags.new_event )
	{
		touch_state.prior_x = ( (touch1->x + touch2->x) / 2 ) / 100;
		touch_state.prior_y = ( (touch1->y + touch2->y) / 2 ) / 100;
	}
	else if( touch1->flags.end_event || touch2->flags.end_event )
	{
	}
	else // touch is a motion, between down and up
	{
		int tmpx;
		int tmpy;
		SetPageOffsetRelative( canvas->current_page
			, touch_state.prior_x - (tmpx=(( (touch1->x + touch2->x) / 2 ) /100))
			, touch_state.prior_y - (tmpy=(( (touch1->y + touch2->y) / 2 ) /100)));
		touch_state.prior_x = tmpx;
		touch_state.prior_y = tmpy;
	}
}

static int CPROC HandleTouch( uintptr_t psv, PINPUT_POINT pTouches, int nTouches )
{
	PCanvasData canvas = (PCanvasData)psv;
	if( nTouches == 2 )
	{
		HandleSingleTouch( canvas, pTouches, pTouches + 1 );
		return TRUE;
	}
	return FALSE;
}
#endif

CONTROL_REGISTRATION menu_surface = { "Menu Canvas"
, { { 512, 460 }, sizeof( CanvasData ), BORDER_WANTMOUSE|BORDER_NONE|BORDER_NOMOVE|BORDER_FIXED }
, NULL //InitMasterFrame
, NULL
, NULL //DrawFrameBackground
, NULL //ShellMouse
, NULL //ShellKey
, NULL, NULL,NULL,NULL,NULL,NULL
, PageFocusChanged
};
CONTROL_REGISTRATION menu_edit_glare = { "Edit Glare"
, { { 512, 460 }, 0, BORDER_WANTMOUSE|BORDER_NONE|BORDER_NOMOVE|BORDER_FIXED }
, NULL //InitMasterFrame
, NULL
, NULL //DrawFrameBackground
, NULL //ShellMouse
, NULL //ShellKey
, NULL, NULL,NULL,NULL,NULL,NULL
, NULL
};


void DisplayMenuCanvas( PSI_CONTROL pc_canvas, PRENDERER under, uint32_t width, uint32_t height, int32_t x, int32_t y )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
	TEXTCHAR title[256];
	PRENDERER banner_rend = GetBanner2Renderer( NULL );
	if( !under )
		under = banner_rend;
	if( !canvas->renderer )
	{
		canvas->renderer = GetFrameRenderer( pc_canvas );
		if( !canvas->renderer )
		{
#ifdef USE_EDIT_GLARE
			Image image = GetControlSurface( g.single_frame );
#endif
			//lprintf( "Opening display at %d,%d %d,%d", x, y, width, height );
			canvas->renderer = OpenDisplayUnderSizedAt( (!g.flags.bTopmost)?under:NULL
																	, g.flags.bTransparent?DISPLAY_ATTRIBUTE_LAYERED:0
																	, width, height, x, y );
			{
				uint32_t c_w, c_h;
				int32_t c_x, c_y;
				GetFrameSize( pc_canvas, &c_w, &c_h );
				GetFramePosition( pc_canvas, &c_x, &c_y );
				if( c_w != width || c_h != height )
               SizeFrame( pc_canvas, width, height );
				if( c_x != x || c_y != y )
               MoveFrame( pc_canvas, x, y );
			}
#ifndef NO_TOUCH
			//lprintf( "Overriding touch handler, this is deprecated, and should get it as a control event instead of from renderer" );
			//SetTouchHandler( canvas->renderer, HandleTouch, (uintptr_t)canvas );
#endif
			GetControlText( pc_canvas, title, 256 );

			SetRendererTitle( canvas->renderer, title );
			AttachFrameToRenderer( pc_canvas, canvas->renderer );
		}
		BindEventToKey( canvas->renderer, KEY_C, KEY_MOD_ALT, DoConfigureKeys, (uintptr_t)canvas );
		BindEventToKey( canvas->renderer, KEY_X, KEY_MOD_ALT, GoodQuitMenu, (uintptr_t)canvas );
		BindEventToKey( canvas->renderer, KEY_F4, KEY_MOD_ALT, GoodQuitMenu, (uintptr_t)canvas );
	}

#ifndef UNDER_CE
#ifdef WIN32
	(((PRENDERER)g.mem_lock)[0]) = canvas->renderer[0];
#endif
#endif

	if( !g.flags.multi_edit )
	{
		HidePageEx( pc_canvas );
		canvas->current_page = NULL;
		ChangePages( pc_canvas, canvas->default_page );
	}
	DisplayFrame( pc_canvas );
}

/* opens a frame per page.... */
PSI_CONTROL OpenPageFrame( PPAGE_DATA page )
{
	// multi edit uses this...
	if( !page->frame )
	{
		static uint32_t xofs, yofs;
		PSI_CONTROL page_frame;
      ProbeDisplaySize();
		page_frame = MakeControl( NULL, menu_surface.TypeID, xofs, yofs, g.default_page_width, g.default_page_height, g.flags.multi_edit?(BORDER_NORMAL|BORDER_RESIZABLE):0 );
		{
			ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, page_frame );
			xofs += 25;
			yofs += 25;
			//SetCommonUserData( page_frame, (uintptr_t)page );
			SetControlText( page_frame, page->title?page->title:"Default Page" );

			SetCommonBorder( page_frame, BORDER_NORMAL|BORDER_RESIZABLE );

			lprintf( "Display menu..." );
			ProbeDisplaySize();
			DisplayMenuCanvas( page_frame, NULL, g.default_page_width, g.default_page_height, g.default_page_x + xofs, g.default_page_y + yofs );
#if 1
			InitSpriteEngine();
#endif
			page->frame = page_frame;
			lprintf( "Page %p is frame %p", page, page_frame );
			canvas->flags.bSuperMode = TRUE;
			return page_frame;
		}
	}
	return page->frame;
}

void InvokeFinishInit( PSI_CONTROL canvas );

PSI_CONTROL Init( LOGICAL bLoadConfig )
{
	PSI_CONTROL pc_canvas;
	if( g.flags.restoreload )
	{
		TEXTCHAR *ext;
		ext = strrchr( g.config_filename, '.' );
		if( !ext || StrCaseCmpEx( ext, ".AutoConfigBackup", 17 ) )
		{
			TEXTCHAR msg[256];
			snprintf( msg, sizeof( msg ), "%s\nINVALID Configuration Name to Restore\nShould be like *.AutoConfigBackup*"
				, g.config_filename );
			Banner2Message( msg );
			return NULL;
		}
		g.flags.forceload = 1; // -restore implies -force
	}

	pc_canvas = SetupSystemsListAndGlobalSingleFrame();

	// Load the previous configuration file...
	// and make the controls found therein...
	{
		PGLARE_SET glare_set;
		if( bLoadConfig )
		{
			Banner2NoWaitAlpha( "Read config..." );
			LoadButtonConfig( pc_canvas, g.config_filename );
		}
		// default images...
		MakeGlareSet( "DEFAULT", NULL, NULL, NULL, NULL );
		//SetGlareSetFlags( "DEFAULT", 0 );
		if( !( glare_set = CheckGlareSet( "round" ) ) ||
			!( glare_set->mask || glare_set->up ||glare_set->down ||glare_set->glare )
			)
		{
			MakeGlareSet( "round", NULL
				, "%resources%/images/round_ridge_up.png"
				, "%resources%/images/round_ridge_down.png"
				, "%resources%/images/round_mask.png" );
			SetGlareSetFlags( "round", GLARE_FLAG_SHADE );
		}
		if( !( glare_set = CheckGlareSet( "square" ) )||
			!( glare_set->mask || glare_set->up ||glare_set->down ||glare_set->glare ) )
		{
			MakeGlareSet( "square"
				, "%resources%/images/glare.jpg"
				, "%resources%/images/ridge_up.png"
				, "%resources%/images/ridge_down.png"
				, "%resources%/images/square_mask.png" );
			SetGlareSetFlags( "square", GLARE_FLAG_SHADE );
		}
		if( !( glare_set = CheckGlareSet( "bicolor square" ) )||
			!( glare_set->mask || glare_set->up ||glare_set->down ||glare_set->glare ) )
		{
			MakeGlareSet( "bicolor square"
				, NULL
				, "%resources%/images/defaultLens.png"
				, "%resources%/images/pressedLens.png"
				, "%resources%/images/colorLayer.png" );
			SetGlareSetFlags( "bicolor square", GLARE_FLAG_MULTISHADE );
		}

		if( bLoadConfig )
		{
			Banner2NoWaitAlpha( "Finish Config..." );
			InvokeFinishInit( pc_canvas );
		}
		if( g.flags.bLogNames )
		{
			DumpRegisteredNames();
		}

		g.flags.bInitFinished = 1;
#ifdef DEBUG_BACKGROUND_UPDATE
		xlprintf(LOG_UPDATE_AND_REFRESH_LEVEL)( "-------------------------------------- BEGIN CHANGE PAGE ----------------------------------------------" );
#endif
	}
	return pc_canvas;
}


PRIORITY_ATEXIT( ExitMisc, ATEXIT_PRIORITY_DEFAULT + 1 )
{
	// not sure how we could be here without exit having
	// been set - perhaps a signal exit?
	InvokeInterShellShutdown();
	{
		PMENU_BUTTON button;
		INDEX idx;
		PPAGE_DATA page;
		INDEX idx2;
		if( !g.flags.multi_edit )
		{
			PSI_CONTROL pc_canvas;
			INDEX idx_canvas;
#ifndef __NO_OPTIONS__
			if( 0 )//SACK_GetProfileIntEx( GetProgramName(), "Destroy Controls at exit", 0, TRUE ) )
#else
			if(0)
#endif
			LIST_FORALL( g.frames, idx_canvas, PSI_CONTROL, pc_canvas )
			{
				InterShell_DisablePageUpdate( pc_canvas, TRUE );
				LIST_FORALL( g.all_pages, idx2, PPAGE_DATA, page )
				{
					ChangePages( page->canvas->pc_canvas, page );
					LIST_FORALL( page->controls, idx, PMENU_BUTTON, button )
					{
						DestroyButton( button );
					}
					if( page->frame != pc_canvas )
					{
						DestroyFrame( &page->frame );
					}
					else
						page->frame = NULL;
				}
				InterShell_DisablePageUpdate( pc_canvas, TRUE );
				//DestroyFrame( &g.single_frame );
			}
		}
		else
		{
			// destory g.frames
		}
		// Destroy popups...
	}
	if( !g.flags.bExit )
	{
		g.flags.bExit = 1;
		WakeThread( g.pMainThread );
	}
}

//------------------------------------------------------
static void OnKeyPressEvent( "InterShell/Show Names" )( uintptr_t psv )
{
	DumpRegisteredNames();
}
static uintptr_t OnCreateMenuButton( "InterShell/Show Names" )( PMENU_BUTTON button )
{
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_ORANGE, BASE_COLOR_BLACK, 0 );
	button->text = StrDup( "Show_Names" );
	button->glare_set = GetGlareSet( "bicolor square" );
	return (uintptr_t)button;
}

void DoEditOptions( void )
{
	int (*EditOptions)( PODBC odbc, PSI_CONTROL parent );
	EditOptions = (int (*)( PODBC odbc,PSI_CONTROL ))LoadFunction( "EditOptions.plugin", "EditOptions" );
	if( EditOptions )
		EditOptions( NULL, NULL );
}
//------------------------------------------------------
static void OnKeyPressEvent( "InterShell/Edit Options" )( uintptr_t psv )
{
	DoEditOptions();
}

static uintptr_t OnCreateMenuButton( "InterShell/Edit Options" )( PMENU_BUTTON button )
{
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_ORANGE, BASE_COLOR_BLACK, 0 );
	button->text = StrDup( "Edit_Options" );
	button->glare_set = GetGlareSet( "bicolor square" );
	return (uintptr_t)button;
}

//------------------------------------------------------
static void OnKeyPressEvent( "InterShell/Generate Exception" )( uintptr_t psv )
{
	*(int*)0 = 0;
}
static uintptr_t OnCreateMenuButton( "InterShell/Generate Exception" )( PMENU_BUTTON button )
{
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_ORANGE, BASE_COLOR_BLACK, 0 );
	button->text = StrDup( "Generate_Exception" );
	button->glare_set = GetGlareSet( "bicolor square" );
	return (uintptr_t)button;
}

//------------------------------------------------------
static void OnKeyPressEvent( "InterShell/Debug Break" )( uintptr_t psv )
{
	DebugBreak();
}
static uintptr_t OnCreateMenuButton( "InterShell/Debug Break" )( PMENU_BUTTON button )
{
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_ORANGE, BASE_COLOR_BLACK, 0 );
	button->text = StrDup( "Debug_Break" );
	button->glare_set = GetGlareSet( "bicolor square" );
	return (uintptr_t)button;
}

static void InvokeQuit( void )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/quit application", &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		void (CPROC*f)(void);
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (void) );
		if( f )
			f();
	}
}


static void ExitKeypress( void )
{
	InvokeQuit();
#ifdef __cplusplus_cli
	//Application::Exit();
#endif
	if( g.flags.bTerminateStayResident )
	{
		BAG_Exit(0xd1e);
	}
	else
	{
		g.flags.bExit = 1;
		WakeThread( g.pMainThread );
		// if it's in a macro, don't continue rest of macro.
		SetMacroResult( FALSE );
	}
}

//------------------------------------------------------
static void OnKeyPressEvent( "Quit Application" )( uintptr_t psv )
{
	Banner2NoWaitAlpha( "Exiting..." );
	ExitKeypress();
}
static uintptr_t OnCreateMenuButton( "Quit Application" )( PMENU_BUTTON button )
{
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_RED, BASE_COLOR_BLACK, 0 );
	button->text = StrDup( "Quit" );
	button->glare_set = GetGlareSet( "bicolor square" );
	return (uintptr_t)button;
}
//------------------------------------------------------

void InterShell_SetTheme( PSI_CONTROL pc_canvas, int ID )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );

	StoreTheme( canvas );
	if( ID != g.theme_index )
	{
		g.theme_index = ID;
		UpdateTheme( canvas );
	}

}


static void OnKeyPressEvent( "InterShell/Next Theme" )( uintptr_t psv )
{
	int tmp = g.theme_index + 1;
	if( tmp >= g.max_themes )
		tmp = 0;
	InterShell_SetTheme( ((PMENU_BUTTON)psv)->canvas->pc_canvas, tmp );
}

static uintptr_t OnCreateMenuButton( "InterShell/Next Theme" )( PMENU_BUTTON button )
{
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE, BASE_COLOR_ORANGE, BASE_COLOR_BLACK, 0 );
	button->text = StrDup( "Next_Theme" );
	button->glare_set = GetGlareSet( "bicolor square" );
	return (uintptr_t)button;
}



void InvokeFinishAllInit( void )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/finish all init", &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		void (CPROC*f)(void);
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (void) );
		if( f )
			f();
	}
}

void InvokeFinishInit( PSI_CONTROL pc_canvas )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	//DumpRegisteredNames();
	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/finish init", &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		void (CPROC*f)(PSI_CONTROL pc);
		//snprintf( rootname, sizeof( rootname ), TASK_PREFIX "/common/save common/%s", name );
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (PSI_CONTROL) );
		if( f )
			f(pc_canvas);
	}
	if( g.pSelectionMenu )
	{
		PMENU pExtraCreate;
		pExtraCreate = MakeControlsMenu( g.pSelectionMenu, TASK_PREFIX "/control", NULL );
		//AppendPopupItem( canvas->pSelectionMenu, MF_STRING|MF_POPUP, (uintptr_t)canvas->pPageMenu, "Change Page" );
		//AppendPopupItem( canvas->pSelectionMenu, MF_STRING|MF_POPUP, (uintptr_t)canvas->pPageDestroyMenu, "Destroy Page" );
		//AppendPopupItem( canvas->pSelectionMenu, MF_STRING|MF_POPUP, (uintptr_t)canvas->pPageUndeleteMenu, "Undestroy Page" );
		AppendPopupItem( g.pSelectionMenu, MF_STRING|MF_POPUP, (uintptr_t)pExtraCreate, "Create other..." );
	}
	// additional global properties might ahve been registered...
	if( !g.global_properties )
	{
		int n = 0;
		CTEXTSTR name;
		PCLASSROOT data = NULL;
		for( name = GetFirstRegisteredName( TASK_PREFIX "/common/global properties", &data );
			name;
			name = GetNextRegisteredName( &data ) )
		{
			void (CPROC*f)(PSI_CONTROL);
			f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (PSI_CONTROL) );
			if( f )
			{
				SetLink( &g.global_properties, n, f );
				SetLink( &g.global_property_names, n, StrDup( name ) );
				n++;
			}
		}
	}
	// additional global security modules might ahve been registered...
	if( !g.security_property_names )
	{
		CTEXTSTR name;
		PCLASSROOT data = NULL;
		for( name = GetFirstRegisteredName( TASK_PREFIX "/common/Security/Edit Security", &data );
			name;
			name = GetNextRegisteredName( &data ) )
		{
			void (CPROC*f)(PSI_CONTROL);
			f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (PSI_CONTROL) );
			if( f )
			{
				AddLink( &g.security_property_names, StrDup( name ) );
			}
		}
	}
	{
		INDEX idx;
		CTEXTSTR name;
		if( !g.pGlobalPropertyMenu )
			g.pGlobalPropertyMenu = CreatePopup();
		LIST_FORALL( g.global_property_names, idx, CTEXTSTR, name )
			AppendPopupItem( g.pGlobalPropertyMenu, MF_STRING, idx + MNU_GLOBAL_PROPERTIES, name );
	}
}

#ifdef USE_EDIT_GLARE
void AddGlareLayer( PCanvasData canvas, Image image )
{
	if( !canvas->edit_glare )
	{
		// must support layered windows.
		// somehow?
		canvas->edit_glare = OpenDisplaySizedAt( DISPLAY_ATTRIBUTE_LAYERED
															 //| DISPLAY_ATTRIBUTE_CHILD
															, g.width, g.height
															, g.x, g.y );
		lprintf( "glare is %p (%p)", canvas->edit_glare, GetNativeHandle( canvas->edit_glare ) );
		SetRendererTitle( canvas->edit_glare, "Edit Glare" );
		canvas->edit_glare_frame = MakeNamedControl( NULL, "Edit Glare", image->x, image->y, image->width, image->height, -1 );
		BindEventToKey( canvas->edit_glare, KEY_ESCAPE, 0, EventAbortConfigureKeys, (uintptr_t)canvas->pc_canvas );
		AttachFrameToRenderer( canvas->edit_glare_frame, canvas->edit_glare );
		SetRedrawHandler( canvas->edit_glare, DrawEditGlare, (uintptr_t)canvas );
		SetMouseHandler( canvas->edit_glare, MouseEditGlare, (uintptr_t)canvas );
		//lprintf( "MAKE yT" );
		MakeTopmost( canvas->edit_glare );
		//lprintf( "Is it topmost?" );
		// just create that thing, don't show it yet...
		//
	}
}
#endif

int restart( void )
{
	static int first_restart = 1;
	PSI_CONTROL pc_canvas = Init( TRUE );
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
#ifdef DEBUG_BACKGROUND_UPDATE
	xlprintf(LOG_UPDATE_AND_REFRESH_LEVEL)( "Displaying the frame on the real display..." );
#endif
	if( !g.flags.multi_edit )
	{
		if( canvas )
		{
			//lprintf( "Making sure we start from NO Page, in case the first page is protected..." );
			//lprintf( "Should we have a rule for default forward, backward, first, last?  From NULL? from another?" );
			HidePageEx( pc_canvas );
			canvas->current_page = NULL;
			ChangePages( pc_canvas, canvas->default_page );
		}
	}
	InvokeStartupMacro();
	Banner2NoWaitAlpha( "and we go..." );
#ifndef __NO_SQL__
	SQLSetFeedbackHandler( NULL );
#endif
	// have at least this one.
	do
	{
		InterShell_DisablePageUpdate( pc_canvas, FALSE );
		if( g.flags.multi_edit )
			SetCommonBorder( canvas->default_page->frame, BORDER_NORMAL|BORDER_RESIZABLE );
		ProbeDisplaySize();
		DisplayMenuCanvas( canvas->default_page->frame, NULL, g.default_page_width, g.default_page_height, g.default_page_x, g.default_page_y );
		if( !g.flags.multi_edit )
		{
#ifndef __NO_ANIMATION__
			InitSpriteEngine( );
			PlayAnimation( pc_canvas );
#endif
			//WakeableSleep( 250 );
			if( g.flags.bTopmost )
			{
            lprintf( "Displaying... topmost also" );
				if( g.flags.bAbsoluteTopmost )
				{
               lprintf( "displaying absolute topmost" );
					MakeAbsoluteTopmost( canvas->renderer );
				}
            else
					MakeTopmost( canvas->renderer );
			}
		}
		else
		{
			INDEX idx;
			PPAGE_DATA page;
			LIST_FORALL( canvas->pages, idx, PPAGE_DATA, page )
			{
				RestorePageEx( GetCanvas( page->frame ), page, TRUE, TRUE );
			}
		}
		if( first_restart )
		{
			first_restart = 0;
			InvokeFinishAllInit();
		}
		{
			PBANNER banner = NULL;
			// this has to wait... until...
			// the first rendering pass is done... cause we're behind it...
			//lprintf( " ---------- remove banner --------" ) ;//
			RemoveBanner2Ex( &banner DBG_SRC );
		}
		if( g.flags.bForceConfigure )
		{
			ConfigureKeys( pc_canvas, 0 );
		}
		if( !g.flags.bTerminateStayResident )
			while( !g.flags.bExit )
			{
				WakeableSleep( 10000 );
			}
	} while( g.flags.bExit == 3 );  // setting exit as 3 is restart trigger.

	// exit = 1 == restart

	if( !g.flags.bTerminateStayResident && ( g.flags.bExit != 1 ) )
	{
		// This is for restart, destroyed everything we know and are
		// then reloads the configuration from the top.
		if( !g.flags.multi_edit )
		{
			xlprintf(LOG_ALWAYS)( "!!!\n!!!\n!!!! Cleanup NEEDS to be finished!!! \n!!!\n!!!\n" );
		}
	}
	return g.flags.bExit;
}

void InterShell_SetButtonHighlight( PMENU_BUTTON button, LOGICAL bEnable )
{
	button = InterShell_GetPhysicalButton( button );
	if( button && !button->flags.bCustom )
		SetKeyHighlight( button->control.key, bEnable );
}

LOGICAL InterShell_GetButtonHighlight( PMENU_BUTTON button )
{
	button = InterShell_GetPhysicalButton( button );
	if( button && !button->flags.bCustom )
		return GetKeyHighlight( button->control.key );
	return 0;
}

PMENU_BUTTON InterShell_GetCurrentlyCreatingButton( void )
{
	return g.CurrentlyCreatingButton;
}

static CTEXTSTR InterShell_GetCurrentlyCreatingButtonType( void )
{
	return g.CurrentlyCreatingButtonType;
}

static void CPROC MyHandleSQLFeedback( CTEXTSTR message )
{
	lprintf( "SQLMessage %s", message );
	//Banner2NoWaitAlpha( message );
}


#ifndef UNDER_CE
#if defined( WIN32 )
PRIORITY_PRELOAD( ProgramLock, DEFAULT_PRELOAD_PRIORITY+2 )
{
	uintptr_t size = 0;
	TEXTCHAR lockname[256];
	TEXTCHAR resource_path[256];
	TEXTCHAR application_title[256];
	snprintf( lockname, sizeof( lockname ), "%s.instance.lock", GetProgramName() );
#ifndef __NO_OPTIONS__
	g.flags.bSQLConfig = SACK_GetProfileIntEx( GetProgramName(), "Use SQL Configuration", 1, TRUE );
	if( g.flags.bSQLConfig )
	{
		SACK_GetProfileStringEx( GetProgramName(), "Use SQL DSN for Configuration", GetDefaultOptionDatabaseDSN()
									  , g.configuration_dsn, 256, TRUE );
		g.configuration_version = SACK_GetProfileIntEx( GetProgramName(), "Use SQL Option Database Version", 4, TRUE );
	}
	SACK_GetProfileStringEx( "InterShell", "Default resource path"
								  , ""
								  , resource_path
								  , sizeof( resource_path ), TRUE );
	lprintf( "Move to resource directory; setup resource path" );
	SACK_GetProfileStringEx( GetProgramName(), "resource path"
#ifdef __ANDROID_
								  , resource_path[0]?resource_path:"."
#elif defined( __LINUX__ )
								  , resource_path[0]?resource_path:"@/../share/SACK"
#else
								  , resource_path[0]?resource_path:"@/../share/SACK"
#endif
								  , resource_path
								  , sizeof( resource_path ), TRUE );
	SACK_GetProfileStringEx( GetProgramName(), "Application Title"
								  , GetProgramName()
								  , application_title
								  , sizeof( application_title ), TRUE );
	g.single_frame_title = SaveText( application_title );
	GetFileGroup( "Resources", resource_path );
	GetFileGroup( "PSI Frames", "%resources%/frames" );
   //if( !g.flags.bNoChangeDirectory )
	//	SetCurrentPath( resource_path );
#endif

#ifndef __LINUX__
	g.mem_lock = OpenSpace( lockname
		, NULL
		//, "memory.delete"
		, &size );
	if( g.mem_lock )
	{
#ifdef WIN32
		PRENDER_INTERFACE pri = GetDisplayInterface();
		pri->_ForceDisplayFront( (PRENDERER)g.mem_lock );
#endif

		lprintf( "Menu already running, now exiting." );
		exit(0);
		Release( g.mem_lock );
	}
	size = 4096;
	// defined by the make system - TARGETNAME
	// only one of a single version of this program?
	g.mem_lock = OpenSpace( lockname
		, NULL
		, &size );
	if( !g.mem_lock )
	{
		lprintf( "Failed to create instance lock region." );
		exit(0);
	}
	else
	{
		lprintf( "opened lock %p %s", g.mem_lock, lockname );
	}
#endif
}
#endif
#endif


void AddTmpPath( TEXTCHAR *tmp )
{
	// if loading a plugin from a path, add that path to the PATH
	CTEXTSTR old_environ = OSALOT_GetEnvironmentVariable( "PATH" );
#ifdef WIN32
#define PATH_SEP_CHAR ';'
#else
#define PATH_SEP_CHAR ':'
#endif
	// safe conversion.
	size_t len = StrLen( tmp );
	CTEXTSTR result;
	if( !( result = StrCaseStr( old_environ, tmp ) ) )
	{
		OSALOT_PrependEnvironmentVariable( "PATH", tmp );
	}
	else
	{
		if( result == old_environ )
		{
			if( result[len] != PATH_SEP_CHAR )
			{
				OSALOT_PrependEnvironmentVariable( "PATH", tmp );
			}
		}
		else if( result[-1] == PATH_SEP_CHAR )
		{
			// is not at the end of the string, and it's not a ;
			// otherwise end of string is OK.
			if( result[len] && ( result[len] != PATH_SEP_CHAR ) )
			{
				OSALOT_PrependEnvironmentVariable( "PATH", tmp );
			}
		}
		else
		{
			OSALOT_PrependEnvironmentVariable( "PATH", tmp );
		}
	}
}

void CPROC LoadAPlugin( uintptr_t psv, CTEXTSTR name, enum ScanFileProcessFlags flags )
{
	TEXTCHAR msg[256];
	snprintf( msg, sizeof( msg ), "Loading Plugin: %s", name );
	SystemLog( msg );
	{
		size_t len;
		if( ( len = StrLen( name ) ) > 17 )
		{
			CTEXTSTR file = pathrchr( name );
			if( !file )
				file = name;
			snprintf( msg, sizeof( msg ), "Loading Plugin...\n...%s"
					  , file
					  );
		}
		else
			snprintf( msg, sizeof( msg ), "Loaded Plugin...\n%s", name );
	}
	Banner2NoWaitAlpha( msg );
#ifdef HAVE_ENVIRONMENT
	if( pathchr( name ) )
	{
		// if loading a plugin from a path, add that path to the PATH
		TEXTCHAR *tmp = StrDup( name );
#ifdef WIN32
#define PATH_SEP_CHAR ';'
#else
#define PATH_SEP_CHAR ':'
#endif
		// safe conversion.
		TEXTCHAR *trim;
		size_t len;
		trim = (TEXTCHAR*)pathrchr( tmp );
		len = trim - tmp;
		trim[0] = 0;
		AddTmpPath( tmp );
	}
#endif
	LoadFunction( name, NULL );
	snprintf( msg, sizeof( msg ), "Loaded Plugin: %s", name );
	SystemLog( msg );
	{
		size_t len;
		if( ( len = StrLen( name ) ) > 17 )
		{
			CTEXTSTR file = pathrchr( name );
			if( !file )
				file = name;
			snprintf( msg, sizeof( msg ), "Loaded Plugin...\n...%s"
					  , file
					  );
		}
		else
			snprintf( msg, sizeof( msg ), "Loaded Plugin...\n%s", name );
	}
	Banner2NoWaitAlpha( msg );
}

void LoadInterShellPlugins( CTEXTSTR mypath, CTEXTSTR mask, CTEXTSTR extra_path )
{
	TEXTCHAR _filename[256];
	TEXTSTR filename = _filename;
	POINTER info = NULL;
	int bLocalPath = 0;
	TEXTCHAR *tmp_path = ExpandPath( mask );
	TEXTCHAR *ext;
	if( !mypath )
	{
		mypath = StrDup( GetProgramPath() );
		bLocalPath = TRUE;
	}
	//lprintf( "Read line from file: %s", tmp_path );
	//lprintf( "Read line from file: %s", mask );
#ifdef REQUIRE_FULLPATH
	if( !IsAbsolutePath( tmp_path ) )
		snprintf( filename, 256, "%s/%s", mypath, tmp_path );
	else
#endif
		snprintf( filename, 256, "%s", tmp_path );

	// save conversion
	ext = (TEXTCHAR*)pathrchr( filename );
	if( ext )
	{
		ext[0] = 0;
		ext++;
	}
	else
	{
		ext = filename;
		filename = (TEXTCHAR*)".";
		//StrCpyEx( filename, ".", sizeof( filename ) );
	}
	//lprintf( "Scanning as [%s] [%s] [%s]", filename, ext+1, extra_path?extra_path:"" );
	{
		CTEXTSTR old_environ = StrDup( OSALOT_GetEnvironmentVariable( "PATH" ) );
		if( extra_path )
		{
			TEXTSTR tmp = ExpandPath( extra_path );
			AddTmpPath( tmp );
			Release( tmp );
		}
		while( ScanFiles( filename, ext, &info, LoadAPlugin, SFF_DEFAULT, 0 ) );
		OSALOT_SetEnvironmentVariable( "PATH", old_environ );
		Release( (POINTER)old_environ );
	}
	Release( tmp_path );
	if( bLocalPath )
		Release( (POINTER)mypath );

}


#ifdef DEKWARE_PLUGIN
uintptr_t CPROC MenuThread( PTHREAD thread )
{
	TEXTCHAR *argv[] = { NULL };
	int argc = 1;
#else
#ifdef __cplusplus
	extern "C"
#endif
PUBLIC( int, Main)( int argc, TEXTCHAR **argv, int bConsole,struct volume* (CPROC *load)( CTEXTSTR filepath, CTEXTSTR userkey, CTEXTSTR devkey )
				,void (CPROC*unload)(struct volume *) )
{
#endif
#ifndef __ANDROID__
	if( bConsole )
		SetSystemLog( SYSLOG_FILE, stderr );
#endif
	{
		int n;
		for( n = 1; n < argc; n++ )
		{
			if( argv[n][0] == '-' )
			{
				// with subcanvas support, this cannot function, sorry
				// we get confused about which menu belongs to which frame
				// some thought will have to be done to figure this one out.
				if( StrCaseCmp( argv[n]+1, "multi" ) == 0 )
					g.flags.multi_edit = 1; // popup mulitiple framed windows instead of full screen mode.
				else if( StrCaseCmp( argv[n]+1, "force" ) == 0 )
					g.flags.forceload = 1;
				else if( StrCaseCmp( argv[n]+1, "restore" ) == 0 )
					g.flags.restoreload = 1;
				else if( StrCaseCmp( argv[n]+1, "SQL" ) == 0 )
					g.flags.bSQLConfig = 1;
				else if( StrCaseCmp( argv[n]+1, "Sysname=" ) == 0 )
					g.system_name = StrDup( argv[n] + 9 );
				else if (StrCaseCmp(argv[n] + 1, "local") == 0) {
					g.flags.local_config = 1; // don't save in sql...
					g.flags.bSQLConfig = 0;
				} 
				else if (StrCaseCmp(argv[n] + 1, "tsr") == 0)
					g.flags.bTerminateStayResident = 1; // return to caller from main instead of exit and idle.
				else if( StrCaseCmp( argv[n]+1, "names" ) == 0 )
					g.flags.bLogNames = 1;
				else if( StrCaseCmp( argv[n]+1, "edit" ) == 0 )
					g.flags.bForceConfigure = 1;
				else if( StrCaseCmp( argv[n]+1, "Options" ) == 0 )
					DoEditOptions();
			}
			else
			{
				TEXTCHAR *varval;
				if( ( varval = strchr( argv[n], '=' ) ) )
				{
					TEXTCHAR *varname = argv[n];
					varval[0] = 0;
					varval++;
					SetVariable( varname, varval );
				}
				else
				{
					if( g.config_filename )
						Release( g.config_filename );
					g.config_filename = StrDup( argv[n] );
				}
			}
		}
	}
	g.pMainThread = MakeThread();
	if( !g.flags.bTerminateStayResident )
	{
		g.flags.bExit = 0;
		while( restart(  ) != 1 );
		QuitMenu( 0, 0 );
#ifdef USE_INTERFACES
		DropDisplayInterface( g.pRenderInterface );
		DropImageInterface( g.pImageInterface );
#endif
		Release( g.mem_lock );
		return 0;
	}
	else
	{
		restart(); // actually start, but it's oneshot.
	}
	return 1;
}

INTERSHELL_NAMESPACE_END

#ifdef __cplusplus_cli
#include <vcclr.h>


namespace sack
{
namespace InterShell
{
	public ref class InterShell_Canvas
	{
		PSI_CONTROL this_frame;
		PCanvasData canvas;
		PRENDERER render;

	public:

		// preload should have already been invoked...
		InterShell_Canvas( System::IntPtr handle )
		{
			//g.flags.bLogNames = 1;
			g.flags.bExternalApplicationhost = 1;
			InvokeDeadstart();
			Init( FALSE );
			//g.UseWindowHandle = handle;
			//ThreadTo( MenuThread, handle );
			{
				render = MakeDisplayFrom( (HWND)((int)handle) );
				RECT r;
				GetClientRect( (HWND)((int)handle), &r );
				this_frame = MakeNamedControl( NULL, "Menu Canvas", 0, 0
						, r.right-r.left+1, r.bottom-r.top+1, -1 );
				AttachFrameToRenderer( this_frame, render );
				DisplayFrame( this_frame );

				canvas = GetCanvas( this_frame );
				canvas->renderer = render;
				{
#ifdef USE_EDIT_GLARE
					Image image = GetControlSurface( this_frame );
					AddGlareLayer( canvas, image );
#endif
				}
				InterShell_DisablePageUpdate( this_frame, FALSE );
				//return (int)this_frame;
			}
			RemoveBanner2Ex( NULL DBG_SRC );
		}

		void Resize( int width, int height )
		{
			SizeDisplay( render, width, height );
			SizeCommon( this_frame, width, height );
			{
				int32_t x, y;
				uint32_t w, h;
				GetDisplayPosition( canvas->renderer, &x, &y, &w, &h );
				MoveSizeDisplay( canvas->edit_glare, x, y, w, h );
			}
		}
		void Focus( void )
		{
			ForceDisplayFocus( render );
		}

		void  Save( System::String^ string )
		{
			pin_ptr<const wchar_t> wch = PtrToStringChars(string);
			size_t convertedChars = 0;
			size_t  sizeInBytes = ((string->Length + 1) * 2);
			errno_t err = 0;
			TEXTCHAR	 *ch = DupWideToText(wch);

			SaveButtonConfig( this_frame, ch );
		}
		void  Load( System::String^ string )
		{
			pin_ptr<const wchar_t> wch = PtrToStringChars(string);
			TEXTCHAR	 *ch = DupWideToText( wch );
			LoadButtonConfig( this_frame, ch );

			Banner2NoWaitAlpha( "Finish Config..." );
			/* this builds menus and junk based on plugins which have been loaded... */
			InvokeFinishInit( this_frame );
			InvokeFinishAllInit();
			InvokeStartupMacro();
			{
				// cleanup banners.
				//PBANNER Null = NULL;
				RemoveBanner2Ex( NULL DBG_SRC );
			}
		}

		TEXTCHAR*  GetCanvasConfig( PSI_CONTROL canvas )
		{
			return "default_intershell.config";
		}
	};
}
}

INTERSHELL_NAMESPACE

#endif

#ifdef _DEFINE_INTERFACE

static struct intershell_interface RealInterShellInterface = {
GetCommonButtonControls
, SetCommonButtonControls
, RestartMenu
, ResumeMenu
, InterShell_GetButtonColors
, InterShell_SetButtonColors
, InterShell_SetButtonColor
, InterShell_SetButtonText
, InterShell_GetButtonText
, InterShell_SetButtonImage
#ifndef __NO_ANIMATION__
//, InterShell_SetButtonAnimation
#endif
, InterShell_CommonImageLoad
, InterShell_CommonImageUnloadByName
, InterShell_CommonImageUnloadByImage
, InterShell_SetButtonImageAlpha
, InterShell_IsButtonVirtual
, InterShell_SetButtonFont
, InterShell_GetCurrentButtonFont
, InterShell_SetButtonStyle
, InterShell_SaveCommonButtonParameters
, InterShell_GetSystemName
, UpdateButtonExx
, ShellGetCurrentPage
, ShellGetNamedPage
, ShellSetCurrentPage
, ShellCallSetCurrentPage
, ShellReturnCurrentPage
, ClearPageList
, InterShell_DisablePageUpdate
, RestoreCurrentPage
, HidePageExx
, InterShell_DisableButtonPageChange
, CreateLabelVariable
, CreateLabelVariableEx
, LabelVariableChanged
, LabelVariablesChanged
, InterShell_HideEx
, InterShell_RevealEx
, GetPageSize
, SetButtonTextField
, AddButtonLayout
, QueryGetControl //InterShell_GetButtonControl
																 , InterShell_GetLabelText
																 , InterShell_TranslateLabelText
																 , InterShell_GetControlLabelText
																 , SelectACanvasFont
																 , BeginCanvasConfiguration
																 , SaveCanvasConfiguration
																 , NULL //SaveCanvasConfiguration_XML
																 , InterShell_GetCurrentConfigHandler
																 , BeginSubConfiguration
																 , EscapeMenuString
																 , InterShell_GetCurrentLoadingControl
																 , InterShell_GetButtonFont
																 , InterShell_GetButtonFontName
																 , InterShell_GetCurrentButton
																 , InterShell_SetButtonFontName
																 , InterShell_GetPhysicalButton
																 , InterShell_SetButtonHighlight
																 , InterShell_GetButtonHighlight
																 , InterShell_TranslateLabelTextEx
																				 , InterShell_CreateControl
																				 , CreateNamedPage
																				 , InterShell_AddCommonButtonConfig
																				 , InterShell_GetCanvas
																				 , InterShell_SetPageLayout
																				 , CreateSomeControl
																				 , InterShell_GetCurrentlyCreatingButton
																				 , InterShell_GetSaveIndent
																				 , BeginSubConfigurationEx
																				 , InterShell_SetTheme
																				 , DisplayMenuCanvas
																				 , InterShell_SetPageColor
																				 , InterShell_GetButtonUserData
																				 , InterShell_GetButtonCanvas
																				 , UseACanvasFont
																				 , InterShell_GetButtonExtension
																				 , SetTextLabelOptions
																				 , InterShell_GetCurrentLoadingCanvas
																				 , InterShell_GetCurrentSavingCanvas
																				 , CreateACanvasFont
																				 , SetupSecurityEdit
																				 , CreateSecurityContext
																				 , CloseSecurityContext
																				 , InterShell_SaveSecurityInformation
																				 , CreateACanvasFont2
																				 , AddSecurityContextToken
																				 , GetSecurityContextTokens
																				 , GetSecurityModules

																				 , InterShell_SetCloneButton
																				 , InterShell_GetCurrentPageName
                                                             , InterShell_GetCurrentlyCreatingButtonType
};

static POINTER CPROC LoadInterShellInterface( void )
{
	return (POINTER)&RealInterShellInterface;
}

static void CPROC UnloadInterShellInterface( POINTER p )
{
}

static void InitInterShell()
{
#ifndef __NO_OPTIONS__
	g.flags.bTopmost = SACK_GetProfileIntEx( GetProgramName(), "Intershell Layout/Display Topmost", 0, TRUE );
	g.flags.bAbsoluteTopmost = SACK_GetProfileIntEx( GetProgramName(), "Intershell Layout/Display Force Topmost", 0, TRUE );
	g.flags.bTransparent = SACK_GetProfileIntEx( GetProgramName(), "Intershell Layout/Display is transparent", 1, TRUE );
	g.flags.bTerminateStayResident = SACK_GetProfileIntEx( GetProgramName(), "Intershell/TSR", 0, TRUE );
	g.flags.bNoAutoStart = !SACK_GetProfileIntEx( GetProgramName(), "Intershell/Auto Start", 1, TRUE );
	g.flags.bNoChangeDirectory = !SACK_GetProfileIntEx( GetProgramName(), "Intershell/change directory to resources", 1, TRUE );
#endif
	//SystemLogTime( SYSLOG_TIME_CPU| SYSLOG_TIME_DELTA );

	g.system_name = GetSystemName(); // Initialized here. Command argument -Sysname= may override.
#ifdef __ANDROID__
	// need to reset some statuses because we're presistant loaded
	g.flags.bExit = 0;
#endif
	{
		TEXTCHAR buf[256];
		snprintf( buf, sizeof( buf ), "%%resources%%/%s.config", GetProgramName() );
		g.config_filename = StrDup( buf );
	}


}

PRIORITY_PRELOAD( StartTSR, DEFAULT_PRELOAD_PRIORITY+5 )
{
	if( !g.flags.bNoAutoStart ) {
		Banner2NoWaitAlpha( "Loading..." );
#ifndef __NO_SQL__
		SQLSetFeedbackHandler( MyHandleSQLFeedback );
#endif
	}
	if( g.flags.bTerminateStayResident )
		restart(); // actually start, but it's oneshot.
}

PRIORITY_PRELOAD( RegisterInterShellInterface, DEFAULT_PRELOAD_PRIORITY-4 )
{
	RegisterInterface( "InterShell", LoadInterShellInterface, UnloadInterShellInterface );

#undef SYMNAME
#define SYMNAME( a,b) EasyRegisterResource( "intershell/visibility", a, b );
		SYMNAME( LIST_ALLOW_SHOW, LISTBOX_CONTROL_NAME )
		SYMNAME( LIST_DISALLOW_SHOW, LISTBOX_CONTROL_NAME )
		SYMNAME( LIST_SYSTEMS, LISTBOX_CONTROL_NAME ) // known systems list
		SYMNAME( EDIT_SYSTEM_NAME, EDIT_FIELD_NAME )
		SYMNAME( BTN_ADD_SYSTEM, NORMAL_BUTTON_NAME )
		SYMNAME( BTN_ADD_SYSTEM_TO_DISALLOW, NORMAL_BUTTON_NAME )
		SYMNAME( BTN_ADD_SYSTEM_TO_ALLOW, NORMAL_BUTTON_NAME )
		SYMNAME( BTN_REMOVE_SYSTEM_FROM_DISALLOW, NORMAL_BUTTON_NAME )
		SYMNAME( BTN_REMOVE_SYSTEM_FROM_ALLOW, NORMAL_BUTTON_NAME )
#undef SYMNAME
#define SYMNAME( a,b) EasyRegisterResource( "intershell/page property", a, b );
		SYMNAME( EDIT_PAGE_GRID_PARTS_X, EDIT_FIELD_NAME )
		SYMNAME( EDIT_PAGE_GRID_PARTS_Y, EDIT_FIELD_NAME )
#undef SYMNAME
#define SYMNAME( a,b) EasyRegisterResource( "intershell/Button General/security", a, b );
		SYMNAME( LISTBOX_SECURITY_MODULE, LISTBOX_CONTROL_NAME );
		SYMNAME( EDIT_SECURITY, NORMAL_BUTTON_NAME );

	EasyRegisterResource( "intershell/glareset", MNU_EDIT_GLARES, "Popup Menu" );
	EasyRegisterResource( "intershell/glareset", LISTBOX_GLARE_SETS, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( "intershell/glareset", LISTBOX_GLARE_SET_THEME, LISTBOX_CONTROL_NAME );
	EasyRegisterResource( "intershell/glareset", EDIT_GLARESET_GLARE, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/glareset", EDIT_GLARESET_UP, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/glareset", EDIT_GLARESET_DOWN, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/glareset", EDIT_GLARESET_MASK, EDIT_FIELD_NAME );
	EasyRegisterResource( "intershell/glareset", CHECKBOX_GLARESET_MULTISHADE, RADIO_BUTTON_NAME );
	EasyRegisterResource( "intershell/glareset", CHECKBOX_GLARESET_SHADE, RADIO_BUTTON_NAME );
	EasyRegisterResource( "intershell/glareset", CHECKBOX_GLARESET_FIXED, RADIO_BUTTON_NAME );
	EasyRegisterResource( "intershell/glareset", GLARESET_APPLY_CHANGES, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "intershell/glareset", GLARESET_CREATE, NORMAL_BUTTON_NAME );
	EasyRegisterResource( "intershell/glareset", GLARESET_ADD_THEME, NORMAL_BUTTON_NAME );

	DoRegisterControl( &menu_surface );
	DoRegisterControl( &menu_edit_glare );

	InitInterShell();
}

static void OnKeyPressEvent( "InterShell/Debug Memory" )( uintptr_t psv )
{
	DebugDumpMem();
}

static uintptr_t OnCreateMenuButton( "InterShell/Debug Memory" )( PMENU_BUTTON button )
{
	return 1;
}

static volatile int dirty_variable_task_done;

static void CPROC TaskEnded( uintptr_t psv, PTASK_INFO task )
{
	dirty_variable_task_done = 1;
}

static void OnKeyPressEvent( "InterShell/Reset SQL Configuration" )( uintptr_t psv )
{
	TEXTCHAR cmd[256];
	CTEXTSTR args[4];
	TEXTCHAR *tmp_path = ExpandPath( g.config_filename );
	args[0] = "@/set_config";
	args[1] = tmp_path;
	args[2] = NULL;
	dirty_variable_task_done = 0;
	if( LaunchProgramEx( args[0], NULL, args, TaskEnded, 0 ) )
	{
		// while not required, would be nice to know when the config is ready to read....
		while( !dirty_variable_task_done )
			Relinquish();

		Banner2NoWaitAlpha( "Restarting..." );
		snprintf( cmd, 256, "@/%s.restart.exe", GetProgramName() );
		args[0] = cmd;
		args[1] = NULL;
		LaunchProgramEx( args[0], NULL, args, TaskEnded, 0 );
	}
	else
		Banner2Message( "Failed..." );
}

static uintptr_t OnCreateMenuButton( "InterShell/Reset SQL Configuration" )( PMENU_BUTTON button )
{
	return 1;
}

static void OnKeyPressEvent( "InterShell/Restart Application" )( uintptr_t psv )
{
	TEXTCHAR cmd[256];
	CTEXTSTR args[4];
	Banner2NoWaitAlpha( "Restarting..." );
	snprintf( cmd, 256, "@/%s.restart.exe", GetProgramName() );
	args[0] = cmd;
	args[1] = NULL;
	if( !LaunchProgramEx( args[0], NULL, args, TaskEnded, 0 ) )
		Banner2Message( "Failed..." );
}

static uintptr_t OnCreateMenuButton( "InterShell/Restart Application" )( PMENU_BUTTON button )
{
	return 1;
}

PUBLIC( void, InvokePreloads )( void )
{
}

INTERSHELL_NAMESPACE_END

#endif
