/*		 M:\work\sack\src\apps\intershell\pages.c
 *  Creator:Jim Buckeyne
 *  Purpose: Page related controls for the POS framework.  Page titles, page changing buttons
 *			  (more)?
 *
 *
 *  (c)Freedom Collective 2006++
 *
 */


/*
 * creates controls:
 *	 page/Page Changer
 *	 page/title
 */
#ifndef INTERSHELL_SOURCE
#define INTERSHELL_SOURCE
#endif

#include <stdhdrs.h> // DebugBreak()
#include <sharemem.h>
#include "intershell_local.h"
#include "resource.h"
#include "intershell_registry.h"
#include <controls.h>
#include "pages.h"
#include "menu_real_button.h"
#include "fileopen.h"
#include "widgets/include/banner.h"
#include "fonts.h"
#include <psi.h>

INTERSHELL_NAMESPACE

	extern CONTROL_REGISTRATION menu_surface;

static struct local_page_information
{
	int current_page_theme; // used in page property dialog to track the current selection
	PPAGE_DATA current_page; // used in page property; saves current page we started on

} local_page_info;

void CreateNamedPage( PSI_CONTROL pc_canvas, CTEXTSTR page_name )
{
	if( pc_canvas )
	{
		PCanvasData canvas = GetCanvas( pc_canvas );
		PPAGE_DATA page = New( PAGE_DATA );
		MemSet( page, 0, sizeof( *page ) );
		page->title = StrDup( page_name );
		page->grid.nPartsX = canvas->current_page?canvas->current_page->grid.nPartsX:90;//canvas->nPartsX;
		page->grid.nPartsY = canvas->current_page?canvas->current_page->grid.nPartsY:50;//canvas->nPartsY;
		AddPage( canvas, page );
		ChangePages( pc_canvas, page );
	}
}

void InsertStartupPage( PSI_CONTROL pc_canvas, CTEXTSTR page_name )
{
	PCanvasData canvas = GetCanvas( pc_canvas );
	PPAGE_DATA page = New( PAGE_DATA );
	// move canvas->default page.
	// now has a name.
	Release( (POINTER)canvas->default_page->title );
	canvas->default_page->title = StrDup( page_name );
	MemSet( page, 0, sizeof( *page ) );
	page->title = NULL; //StrDup( "New Startup?" );
	// new default page...
	AddPage( canvas, canvas->default_page ); // which is a new page....  but it's got a goofy name... menus suck.
	canvas->default_page = page;
	page->grid.nPartsX = canvas->current_page->grid.nPartsX;
	page->grid.nPartsY = canvas->current_page->grid.nPartsY;
	ChangePages( pc_canvas, page );
}

//-------------------------------------------------------------------------

PPAGE_DATA GetCurrentCanvasPage( PCanvasData canvas )
{
	if( canvas )
		return canvas->current_page;
	return NULL;
}

//-------------------------------------------------------------------------

CTEXTSTR InterShell_GetCurrentPageName( PSI_CONTROL pc_canvas )
{
	PCanvasData canvas = GetCanvas( pc_canvas );
	PPAGE_DATA page = GetCurrentCanvasPage( canvas );
	return page->title?page->title:"first";
}

//-------------------------------------------------------------------------

PPAGE_DATA ShellGetNamedPage( PSI_CONTROL pc_canvas, CTEXTSTR name )
{
	if( pc_canvas )
	{
		PCanvasData canvas = GetCanvas( pc_canvas );
		INDEX idx;
		PPAGE_DATA page;
		if( !canvas )
			return NULL;
		if( strcmp( name, "first" ) == 0 )
		{
			return canvas->default_page;
		}
		if( strcmp( name, "next" ) == 0 )
		{
			INDEX idx_page = FindLink( &canvas->pages, canvas->current_page );
			if( idx_page == INVALID_INDEX )
			{
				INDEX idx_first;
				PPAGE_DATA page;
				LIST_FORALL( canvas->pages, idx_first, PPAGE_DATA, page )
				{
					return page;
				}
				return NULL;
			}
			else
			{
				INDEX idx_first = idx_page;
				PPAGE_DATA page;
				LIST_NEXTALL( canvas->pages, idx_first, PPAGE_DATA, page )
				{
					return page;
				}
				return canvas->default_page;
			}
		}
		if( strcmp( name, "here" ) == 0 )
		{
			return canvas->current_page;
		}
		else if( strcmp( name, "return" ) == 0 )
		{
			PPAGE_DATA page = (PPAGE_DATA)PopLink( &canvas->prior_pages );
			g.flags.bPageReturn = 1;
			if( page )
			{
				return page;
			}
			return canvas->current_page;
		}
		else LIST_FORALL( canvas->pages, idx, PPAGE_DATA, page )
		{
			if( page->title )
				if( strcmp( page->title, name ) == 0 )
				{
									return page;
				}
		}
	}
	return NULL;
}

//-------------------------------------------------------------------------

PPAGE_DATA ShellGetCurrentPage( PSI_CONTROL pc_canvas )
{
	if( pc_canvas )
	{
		PCanvasData canvas = GetCanvas( pc_canvas );
		// focused page...
		// something fun like that.
		if( canvas )
			return canvas->current_page;
	}
	return NULL;
}

//-------------------------------------------------------------------------

void ClearPageList( PSI_CONTROL pc_canvas )
{
	if( pc_canvas )
	{
		PCanvasData canvas = GetCanvas( pc_canvas );
		while( PopLink( &canvas->prior_pages ) );
	}
	//while( PopLink( &local_page_info.prior_pages ) );
}

//-------------------------------------------------------------------------

int InvokePageChange( PSI_CONTROL pc_canvas )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/change page", &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		int (CPROC*f)(PSI_CONTROL);
		f = GetRegisteredProcedure2( data, int, name, (PSI_CONTROL) );
		if( f )
			if( !f( pc_canvas ) )
				break;
	}
	if( name )
		return FALSE;
	return TRUE;
}

//-------------------------------------------------------------------------

void UpdateButtonExx( PMENU_BUTTON button, int bEndingEdit DBG_PASS )
#define UpdateButtonEx( button, edit ) UpdateButtonExx( button, edit DBG_SRC )
{
	int bShow;
	//_xlprintf( LOG_NOISE DBG_RELAY )( "Begin update (%s) button.... (from somwhere)", button->pTypeName );
	if( !bEndingEdit )
	{
		/* better to validate this, so off-page controls don't accidentatlly
		 * show themselves with update.
		 */
		PCanvasData canvas = GetCanvas( GetParentControl( QueryGetControl( button ) ) );
		// doesn't matter ... we're not on this button's page..

		//lprintf( "probably not g.flags.multi_edit ( %d )",  g.flags.multi_edit );
		//lprintf( "real button page %p is %p ?", InterShell_GetPhysicalButton( button )->page, canvas->current_page );
		if( !g.flags.multi_edit && InterShell_GetPhysicalButton( button )->page != canvas->current_page )
			return;
	}

	if( button->flags.bInvisible )
	{
		//lprintf( "invisible buttons (macro components... ) special process )" );
		// call the showcontrol on it... it might know it's parent container to update colors...
		InvokeShowControl( button );
		//lprintf( "Okay... then what ? smudge this?" );
		FlushToKey( InterShell_GetPhysicalButton( button ) );
		SmudgeCommon( QueryGetControl( InterShell_GetPhysicalButton( button ) ) );
		return;
	}

	if( /*button->flags.bInvisible ||*/ ( button->page && !button->page->flags.bActive ) )
	{
		return; // nothing to do if not in a active page.
	}
	// if it's not on a page, it might just be in a macro...
	if( !button->page && !button->container_button )
	{
		//lprintf( "Somehow a button is not on a page...(DEBUG)" );
		DebugBreak();
	}
	if( button->psvUser &&
		 !button->flags.bInvisible && QueryShowControl( button ) )
	{
		if( g.flags.bInitFinished )
		{
			//lprintf( "Show control!.. final fixup? ");
			InvokeShowControl( button );
		}
		bShow = 1;
	}
	else
		bShow = 0;
		
	if( button->flags.bCustom )
	{
		TEXTCHAR rootname[256];
		void (CPROC*f)(uintptr_t);
		if( bShow )
		{
			if( g.flags.bInitFinished )
			{
				//lprintf( "Show control!.. final fixup? ");
				InvokeShowControl( button );
			}
			if( button->flags.bConfigured )
				FixupButton( button );
			//lprintf( "Show control!.. final fixup? ");
			//bShow = 1;
			RevealCommon( QueryGetControl( button ) );
		}
		else
		{
			HideControl( QueryGetControl( button ) );
			//bShow = 0;
		}
		snprintf( rootname
				  , sizeof( rootname )
				  , TASK_PREFIX "/button/%s"
				  , button->pTypeName );
		f = GetRegisteredProcedure2( rootname, void, "show_button", (uintptr_t) );
		if( f )
			f( button->psvUser );
		// button is a key
		if( bEndingEdit )
			InvokeEditEnd( button ); // button may not want to be shown.
	}
	else
	{
		//lprintf( "Revealing a key (by invoking it's editend method)");
		// button is a key
		//lprintf( "Fixup should display the button...");
		if( bShow )
		{
			if( bEndingEdit  )
			{
				InvokeEditEnd( button ); // button may not want to be shown.
			}
			FixupButton( button );
		}
		else
		{
			HideControl( QueryGetControl( button ) );
		}
	}
	// if the button doesn't have a purpose, then
	// never show it... (purpose defined by the setting of psvUser to
	// some plugin's use data...
	//lprintf( " button type is %s and psv is %p", button->pTypeName, button->psvUser );
	/*
	if( bShow )
	{
		RevealCommon( QueryGetControl( button ) );
	}
	else
	{
		HideControl( QueryGetControl( button ) );
		}
		*/
}

// added pc_canvas very late to supprot shellgetnamedpage
void RestorePageEx( PCanvasData canvas, PPAGE_DATA page, int bFull, int was_active )
{
	PPAGE_DATA prior;
	INDEX idx;
	PMENU_BUTTON control;
	//_lprintf(DBG_RELAY)( "restore page... %d", was_active );
	if( g.flags.multi_edit )
	{
		//if( page->frame )  // why wouldn't a page have a frame?
		//	DisplayFrame( page->frame );
	}
	if( canvas->pPageMenu )
		CheckPopupItem( canvas->pPageMenu, MNU_CHANGE_PAGE + page->ID, MF_CHECKED );
	prior = canvas->current_page;
	do
	{
		//lprintf( "page set to %p", page );
		canvas->current_page = page;
		if( !InvokePageChange( canvas->pc_canvas ) ) // some method rejected page access.
		{
			if( !prior )
			{
				page = ShellGetNamedPage( page->frame, "next" );
				if( !page || page == canvas->default_page )
				{
					Banner2Message( "Failure to find an accessable page, exiting." );
					exit(0);
				}
			}
			//lprintf( "page set to %p", prior );
			canvas->current_page = prior;
			if( prior && !InvokePageChange( canvas->pc_canvas ) ) // some method rejected page access.
			{
				// we have to be sure we can be on this page too, since we 'left'
				// that is we went to another page..
				//lprintf( "page set to %p", NULL );
				canvas->current_page = prior = NULL;
			}
		}
	} while( !canvas->current_page );
	if( was_active )
	{
		if( canvas->current_page )
		{
			canvas->current_page->flags.bActive = 1;
			LIST_FORALL( canvas->current_page->controls, idx, PMENU_BUTTON, control )
			{
				// full flag will cause the control to get reinitialized
				// all parameters will get set.
				UpdateButtonEx( control, bFull );
			}
		}
	}
}
//-------------------------------------------------------------------------

void RestoreCurrentPage( PSI_CONTROL pc_canvas )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
	RestorePage( canvas, canvas->current_page, FALSE );
}

//-------------------------------------------------------------------------

/* this is function has a duplicately named function in main.c */
static void CPROC PageChooseImage( uintptr_t psv, PSI_CONTROL button )
{
	TEXTCHAR buffer[256];
	// was attempting to make a general select here
	// so that it would find smoe related text field
	// but then the button needs to register a style sheet with PSI property dialog...
	// need to make some other things, maybe some in-InterShell controls for editing button glares?

	if( SelectExistingFile( button, buffer, sizeof( buffer ), "*.png\t*.gif\t*.jpg" ) )
		SetControlText( GetNearControl( button, TXT_IMAGE_NAME ), buffer );

}

static void CPROC AddPageTheme( uintptr_t psv, PSI_CONTROL button )
{
	if( g.max_themes < 2 )
	{
		AddTheme( 1 );
	}
	else
	{
		AddTheme( g.max_themes );
	}

	{
		TEXTCHAR buf[32];
		int n;
		for( n = 0; n < g.max_themes; n++ )
		{
			snprintf( buf, 32, "Theme %d", g.max_themes - 1 );
			SetItemData( AddListItem( GetNearControl( button, LISTBOX_PAGE_THEME ), buf ), g.max_themes - 1 );
		}
	}
}

//-------------------------------------------------------------------------
/* this is function has a duplicately named function in main.c */
static void CPROC ChooseAnimation( uintptr_t psv, PSI_CONTROL button )
{
	TEXTCHAR buffer[256];
	// was attempting to make a general select here
	// so that it would find smoe related text field
	// but then the button needs to register a style sheet with PSI property dialog...
	// need to make some other things, maybe some in-InterShell controls for editing button glares?

	if( SelectExistingFile( button, buffer, sizeof( buffer ),"*.mng" ) )
		SetControlText( GetNearControl( button, TXT_ANIMATION_NAME ), buffer );

}

//-------------------------------------------------------------------------

void HidePageExx( PSI_CONTROL pc_canvas DBG_PASS )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
	if( g.flags.multi_edit )
	{
		return;
	}
	if( canvas && canvas->current_page
//		&& canvas->current_page->flags.bActive
	  )
	{
		INDEX idx;
		PMENU_BUTTON control;
		PPAGE_DATA page;
		if( !canvas->current_page->flags.bActive )
			_lprintf(DBG_RELAY)( "hiding a non active page" );
		if( canvas->current_page )
			canvas->current_page->flags.bActive = 0;
		if( canvas->pPageMenu )
			CheckPopupItem( canvas->pPageMenu, MNU_CHANGE_PAGE + canvas->current_page->ID, MF_UNCHECKED );
		if( canvas->current_page )
		{
			page = canvas->current_page;
			//lprintf( "Hiding a page... hiding all controls... controls have the option to cause themselves to show... " );
			LIST_FORALL( page->controls, idx, PMENU_BUTTON, control )
			{
				HideControl( QueryGetControl( control ) );
			}
		}
		//_lprintf(DBG_RELAY)( "page set to %p", NULL );
		//canvas->current_page = NULL;
	}
}

//-------------------------------------------------------------------------

void ChangePagesEx( PSI_CONTROL pc_canvas, PPAGE_DATA page DBG_PASS )
{
	PCanvasData canvas = GetCanvas( pc_canvas );
	// page becomes the new current page... the current page
	// is disabled, adn the new page reenabled..
	static BIT_FIELD bChanging; // : 1; // already changing a page.

	if( bChanging )
	{
		xlprintf(LOG_ALWAYS)( "Page change dropped, was already changing pages" );
		return;
	}
	bChanging = TRUE;
	if( g.flags.multi_edit )
	{
		RestorePage( canvas, page, FALSE );
		ForceDisplayFront( GetFrameRenderer( page->frame ) );
		lprintf( "Someone requested a switch page... perhaps we should entertain doing some display thing to set focus to the page..." );
		bChanging = FALSE;
		return;	// don't hide any controls on any page....
	}
	if( page == canvas->current_page )
	{
		lprintf( "current page is already page (no change)" );
		SmudgeCommon( canvas->pc_canvas );
		bChanging = FALSE;
		return;
	}

	//DumpFrameContents( g.frame );
	//lprintf( "-------------------------------------- ChangePages -------------------------------------" );
	bChanging = TRUE;
	if( !g.flags.bPageUpdateDisabled )
		EnableCommonUpdates( canvas->pc_canvas, FALSE );

	if( !g.flags.bPageReturn && ( canvas->current_page != page ) )
	{
		PushLink( &canvas->prior_pages, canvas->current_page );
	}

	{
		int was_active;
		//_lprintf(DBG_RELAY)( "%p ", canvas->current_page );
		if( !canvas->current_page || ( canvas->current_page && canvas->current_page->flags.bActive ) )
			was_active = 1;
		else
			was_active = 0;
		HidePageEx( canvas->pc_canvas );
		//canvas->current_page = page;
		RestorePageEx( canvas, page, FALSE, was_active);
	}

	//lprintf( "================ SOMETHING SHOULD HAPPEN HERE ===========================" );
	// I dunno about all this....
	if( !g.flags.bPageUpdateDisabled )
	{
		EnableCommonUpdates( canvas->pc_canvas, TRUE );
		/* disabled auto smudge on enable(TRUE).... so smudge */
		SmudgeCommon( canvas->pc_canvas );
	}
	//lprintf( "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ Changed Pages ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^-" );
	bChanging = FALSE;
}

void ShellReturnCurrentPage( PSI_CONTROL pc_canvas )
{
	if( pc_canvas )
	{
		PCanvasData canvas = GetCanvas( pc_canvas );
		PPAGE_DATA page = (PPAGE_DATA)PopLink( &canvas->prior_pages );
		if( page )
		{
			ChangePages( page->frame, page );
		}
	}
}

void SetCurrentPageID( PSI_CONTROL pc_canvas, uint32_t ID )
{
	PCanvasData canvas = GetCanvas( pc_canvas );
	INDEX idx;
	PPAGE_DATA page;
	LIST_FORALL( canvas->pages, idx, PPAGE_DATA, page )
	{
		if( page->ID == ID )
		{
			ChangePages( pc_canvas, page );
			return;
		}
	}
	ChangePages( pc_canvas, canvas->default_page );
}

void DestroyPage( PCanvasData canvas, PPAGE_DATA page )
{
	// shouldn't actually destroy, enque into things
	PLIST controls;
	PMENU_BUTTON button;
	INDEX idx;
	controls = page->controls;
	page->controls = NULL; // clear this out, just in case
	LIST_FORALL( page->controls, idx, PMENU_BUTTON, button )
	{
		DestroyButton( button );
	}
	DeleteLink( &canvas->pages, page );
	DeleteLink( &g.all_pages, page );
	if( g.flags.multi_edit )
	{
		INDEX idx;
		PSI_CONTROL page_frame;
		LIST_FORALL( g.frames, idx, PSI_CONTROL, page_frame )
		{
			PPAGE_DATA check_page = (PPAGE_DATA)GetControlUserData( page_frame );
			if( page == check_page )
			{
				DeleteLink( &g.frames, page_frame );
				DestroyControl( page_frame );
			}
		}
	}
	Release( page );
}

void DestroyPageID( PSI_CONTROL pc_canvas, uint32_t ID ) // MNU_DESTROY_PAGE ID (minus base)
{
	PCanvasData canvas = GetCanvas( pc_canvas );
	INDEX idx;
	PPAGE_DATA page;
	LIST_FORALL( canvas->pages, idx, PPAGE_DATA, page )
	{
		if( page->ID == ID )
		{
			if( canvas->current_page == page )
				ChangePages( pc_canvas, canvas->default_page );

			AddLink( &canvas->deleted_pages, page );
			DeleteLink( &canvas->pages, page );
			DeletePopupItem( canvas->pPageMenu, MNU_CHANGE_PAGE + page->ID, 0 );
			DeletePopupItem( canvas->pPageDestroyMenu, MNU_DESTROY_PAGE + page->ID, 0 );
			AppendPopupItem( canvas->pPageUndeleteMenu, MF_STRING, MNU_UNDELETE_PAGE + page->ID
					, page->title );
			break;
		}
	}
}

void UnDestroyPageID( PSI_CONTROL pc_canvas, uint32_t ID ) // MNU_DESTROY_PAGE ID (minus base)
{
	PCanvasData canvas = GetCanvas( pc_canvas );
	INDEX idx;
	PPAGE_DATA page;
	LIST_FORALL( canvas->deleted_pages, idx, PPAGE_DATA, page )
	{
		if( page->ID == ID )
		{
			if( canvas->current_page == page )
				ChangePages( pc_canvas, canvas->default_page );

			AddLink( &canvas->pages, page );
			DeleteLink( &canvas->deleted_pages, page );
			AppendPopupItem( canvas->pPageMenu, MF_STRING, MNU_CHANGE_PAGE + page->ID, page->title );
			AppendPopupItem( canvas->pPageDestroyMenu, MF_STRING, MNU_DESTROY_PAGE + page->ID, page->title );
			DeletePopupItem( canvas->pPageUndeleteMenu, MF_STRING, MNU_UNDELETE_PAGE + page->ID );
			break;
		}
	}
}


int ShellCallSetCurrentPage( PSI_CONTROL pc_canvas, CTEXTSTR name )
{
	if( pc_canvas )
	{
		return ShellSetCurrentPage( pc_canvas, name );
	}
	return 0;
}

int ShellSetCurrentPage( PSI_CONTROL pc_canvas, CTEXTSTR name )
{
	//ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
	//INDEX idx;
	PPAGE_DATA page;
	if( !name )
	{
		SmudgeCommon( pc_canvas );
		return TRUE;
	}
	page = ShellGetNamedPage( pc_canvas, name );
	if( page )
	{
		ChangePages( pc_canvas, page );
		g.flags.bPageReturn = 0;
		return TRUE;
	}
	return FALSE;
}


PSI_CONTROL SelectTextWidget( void )
{
	PSI_CONTROL frame = LoadXMLFrame( "SelectFileButton.isFrame" );
	{
		//InitListbox( ) ;
	}
	return frame;
}

//---------------------------------------------------------------------------

void AdjustControlPositions( PCanvasData canvas, PPAGE_DATA page )
{
	INDEX idx;
	PSI_CONTROL pc;
	PMENU_BUTTON control;
	LIST_FORALL( page->controls, idx, PMENU_BUTTON, control )
	{
		uint32_t w, h;
		int32_t x, y;
		pc = QueryGetControl( control );
		GetFrameSize( pc, &w, &h );
		GetFramePosition( pc, &x, &y );
		lprintf( "Input control was (real) %d,%d %d,%d", x, y, w, h );
		lprintf( "Input control was (part) %Ld,%Ld %Ld,%Ld", control->x, control->y, control->w, control->h );
		control->x = COMPUTEPARTOFX( x, page->grid.nPartsX );
		control->y = COMPUTEPARTOFY( y, page->grid.nPartsY );
		control->w = COMPUTEPARTOFX( w, page->grid.nPartsX );
		control->h = COMPUTEPARTOFY( h, page->grid.nPartsY );
		lprintf( "Output control was %Ld,%Ld %Ld,%Ld", control->x, control->y, control->w, control->h );
		lprintf( "Output real is %Ld,%Ld %Ld,%Ld", PARTX( control->x ), PARTY( control->y )
							, PARTW( control->x, control->w ), PARTH( control->y, control->h ) );
		MoveSizeControl( QueryGetControl( control ), PARTX( control->x ), PARTY( control->y )
							, PARTW( control->x, control->w ), PARTH( control->y, control->h ) );
	}
}

//---------------------------------------------------------------------------

static void CPROC ListBoxThemeSelectionChanged( uintptr_t psv, PSI_CONTROL list, PLISTITEM pli )
{
	TEXTCHAR prior_name[256];
	int n = (int)GetItemData( pli );
	CDATA prior_color;
	CTEXTSTR prior_image_name;
	GetControlText( GetNearControl( list, TXT_IMAGE_NAME ), prior_name, sizeof( prior_name ) );
	prior_color = GetColorFromWell( GetNearControl( list, CLR_BACKGROUND ) );
	prior_image_name = (CTEXTSTR)GetLink( &local_page_info.current_page->backgrounds, local_page_info.current_page_theme );
	if( prior_image_name )
	{
		if( StrCaseCmp( prior_image_name, prior_name ) != 0 )
		{
			Deallocate( CTEXTSTR, prior_image_name );
			SetLink( &local_page_info.current_page->backgrounds, local_page_info.current_page_theme, StrDup( prior_name ) );
			SetLink( &local_page_info.current_page->background_images, local_page_info.current_page_theme, NULL );
		}
	}

	SetLink( &local_page_info.current_page->background_colors, local_page_info.current_page_theme, (POINTER)(uintptr_t)prior_color );

	local_page_info.current_page_theme = n;

	SetControlText( GetNearControl( list, TXT_IMAGE_NAME ), (CTEXTSTR)GetLink( &local_page_info.current_page->backgrounds, n ) );
	SetColorWell( GetNearControl( list, CLR_BACKGROUND ), (CDATA)(uintptr_t)GetLink( &local_page_info.current_page->background_colors, n ) );
}

//---------------------------------------------------------------------------

void EditCurrentPageProperties(PSI_CONTROL parent, PCanvasData canvas)
{
	//uintptr_t CPROC ConfigurePaper( uintptr_t psv, PMENU_BUTTON button )
//	if(0)
{
	// psv may be passed as NULL, and therefore there was no task assicated with this
	// button before.... the button is blank, and this is an initial creation of a button of this type.
	// basically this should call (psv=CreatePaper(button)) to create a blank button, and then launch
	// the config, and return the button created.
	//PMENU_BUTTON button;
	//uintptr_t psv;
	PSI_CONTROL frame = LoadXMLFrameOver(  parent, "InterShellPageProperty.isFrame" );
	//PPAPER_INFO issue = button->paper;
	//int created = 0;
	int okay = 0;
	int done = 0;
	int _rows, _cols;
	if( !frame )
	{
		frame = CreateFrame( "Page Properties", 0, 0, 420, 250, 0, NULL );
		if( frame )
		{
			PSI_CONTROL button;
			MakeTextControl( frame, 5, 97, 120, 18, TXT_STATIC, "Background", 0 );
			EnableColorWellPick( MakeColorWell( frame, 130, 97, 18, 18, CLR_BACKGROUND, canvas->current_page->background_color ), TRUE );

			MakeEditControl( frame, 130, 120, 240, 18, TXT_IMAGE_NAME, canvas->current_page->background, 0 );
			button = MakeButton( frame, 89, 120, 36, 18, BTN_PICKFILE, "...", 0, PageChooseImage, (uintptr_t)frame );

			MakeEditControl( frame, 130, 143, 240, 18, TXT_ANIMATION_NAME, canvas->current_page->background, 0 );
			button = MakeButton( frame, 89, 143, 36, 18, BTN_PICKANIMFILE, "...", 0, ChooseAnimation, (uintptr_t)frame );

			SaveXMLFrame( frame, "InterShellPageProperty.isFrame" );
					//SetControlUserData( button, local_page_info.file_text_field );

			/*
			AddPropertySheet( button
								 , SelectFileOutputTextWidget
								 , ApplySelectFileOutputTextWidget
								 );
			*/
			//MakeTextControl( frame, 5, 143, 120, 18, TXT_STATIC, "Text color", 0 );
			//EnableColorWellPick( MakeColorWell( frame, 130, 143, 18, 18, CLR_TEXT_COLOR, button->textcolor ), TRUE );

			AddCommonButtons( frame, &done, &okay );
		}
	}

	if( frame )
	{
		local_page_info.current_page = canvas->current_page;
		local_page_info.current_page_theme = 0;

		SetCommonButtons( frame, &done, &okay );
		EnableColorWellPick( GetControl( frame, CLR_BACKGROUND ), TRUE );
		SetColorWell( GetControl( frame, CLR_BACKGROUND ), canvas->current_page->background_color );
		SetButtonPushMethod( GetControl( frame, BTN_PICKFILE ), PageChooseImage, (uintptr_t)frame );
		SetControlText( GetControl( frame, TXT_IMAGE_NAME ), canvas->current_page->background );

		SetButtonPushMethod( GetControl( frame, BTN_PICKANIMFILE ), ChooseAnimation, (uintptr_t)frame );
		SetControlText( GetControl( frame, TXT_ANIMATION_NAME ), canvas->current_page->background );
		SetButtonPushMethod( GetControl( frame, BTN_ADD_PAGE_THEME ), AddPageTheme, (uintptr_t)frame );
		{
			TEXTCHAR buf[32];
			int n;
			for( n = 0; n < g.max_themes; n++ )
			{
				if( !n )
					snprintf( buf, 32, "Default Theme" );
				else
					snprintf( buf, 32, "Theme %d", n );
				SetItemData( AddListItem( GetControl( frame, LISTBOX_PAGE_THEME ), buf ), n );
			}
			SetSelChangeHandler( GetControl( frame, LISTBOX_PAGE_THEME ), ListBoxThemeSelectionChanged, (uintptr_t)frame );
		}
		{
			TEXTCHAR buffer[25];
			snprintf( buffer, sizeof( buffer ), "%d", _cols = canvas->current_page->grid.nPartsX );
			SetControlText( GetControl( frame, EDIT_PAGE_GRID_PARTS_X ), buffer );
			snprintf( buffer, sizeof( buffer ), "%d", _rows = canvas->current_page->grid.nPartsY );
			SetControlText( GetControl( frame, EDIT_PAGE_GRID_PARTS_Y ), buffer );
		}
	}
	//DisplayFrame( frame );
	DisplayFrameOver( frame, parent );
	CommonWait( frame );

	lprintf( "Wait complete... %d %d", okay, done );
	if( okay )
	{
		TEXTCHAR buffer[256];
		GetControlText( GetControl( frame, TXT_IMAGE_NAME ), buffer, sizeof( buffer ) );
		// Get info from dialog...
		canvas->current_page->background_color = GetColorFromWell( GetControl( frame, CLR_BACKGROUND ) );
		SetLink( &local_page_info.current_page->background_colors, local_page_info.current_page_theme, (POINTER)(uintptr_t)canvas->current_page->background_color );
		if( buffer[0] )
		{
			canvas->current_page->background = StrDup( buffer );
			SetLink( &canvas->current_page->backgrounds, local_page_info.current_page_theme, (POINTER)(uintptr_t)canvas->current_page->background );
			if( canvas->current_page->background_image )
			{
				UnmakeImageFile( canvas->current_page->background_image );
				canvas->current_page->background_image = NULL;
				//SmudgeCommon( parent );
			}
		}
		else
		{
			if( canvas->current_page->background )
			{
				SetLink( &canvas->current_page->backgrounds, local_page_info.current_page_theme, NULL );
				Release( (POINTER)canvas->current_page->background );
				canvas->current_page->background = NULL;
				UnmakeImageFile( canvas->current_page->background_image );
				canvas->current_page->background_image = NULL;
				//SmudgeCommon( parent );
			}
		}
		GetControlText( GetControl( frame, EDIT_PAGE_GRID_PARTS_X ), buffer, sizeof( buffer ) );
		canvas->current_page->grid.nPartsX = atoi( buffer );
		GetControlText( GetControl( frame, EDIT_PAGE_GRID_PARTS_Y ), buffer, sizeof( buffer ) );
		canvas->current_page->grid.nPartsY = atoi( buffer );
 		if( (_rows != canvas->current_page->grid.nPartsY) ||
			(_cols != canvas->current_page->grid.nPartsX) )
		{
			AdjustControlPositions( canvas, canvas->current_page );
			/* assume things have updated, color, etc... during configuration this redraw shouldn't be an issue*/
			SmudgeCommon( parent );
		}
		//button->textcolor = GetColorFromWell( GetControl( frame, CLR_TEXT_COLOR ) );
	}
	lprintf( "destroying frame" );
	DestroyFrame( &frame );
	lprintf( "Destroyed frame" );
//	return psv;
}


}

//-------------------------------------------------------------------------

void AddPage( PCanvasData canvas, PPAGE_DATA page )
{
	static int nPage = 1;
	AddLink( &canvas->pages, page );
	AddLink( &g.all_pages, page );
	page->canvas = canvas;
	if( nPage < MNU_CHANGE_PAGE_MAX - MNU_CHANGE_PAGE )
	{
		AppendPopupItem( canvas->pPageMenu, MF_STRING, MNU_CHANGE_PAGE + nPage, page->title );
	}
	else
		lprintf( "Failed to add new item %s to page menu (more than %d pages!)", page->title?page->title:"DEFAULT PAGE", nPage );
	if( nPage < MNU_DESTROY_PAGE_MAX - MNU_DESTROY_PAGE )
	{
		AppendPopupItem( canvas->pPageDestroyMenu, MF_STRING, MNU_DESTROY_PAGE + nPage, page->title );
	}
	else
		lprintf( "Failed to add new item %s to page destroy menu (more than %d pages!)", page->title, nPage );
	page->ID = nPage; // this is a pretty crazy ID - should consider rebuilding this.
	nPage++;
	if( g.flags.multi_edit )
		OpenPageFrame( page );
	else
		page->frame = canvas->pc_canvas;
}

//-------------------------------------------------------------------------

PPAGE_DATA GetPageFromFrame( PSI_CONTROL pc_canvas )
{
	ValidatedControlData( PCanvasData, menu_surface.TypeID, canvas, pc_canvas );
	if( canvas )
		return canvas->current_page;
	return NULL;
}

//---------------------------------------------------------------------------

PPAGE_DATA CreateAPage( void )
{
	PPAGE_DATA page = New( PAGE_DATA );
	MemSet( page, 0, sizeof( *page ) );
	return page;
}

//---------------------------------------------------------------------------

void CreateNewPage( PSI_CONTROL pc_canvas, PCanvasData canvas )
{
	TEXTCHAR pagename[256];
	if( SimpleUserQuery( pagename, sizeof( pagename ), "Enter Page Name"
							 , pc_canvas ) )
	{
		PPAGE_DATA page = New( PAGE_DATA );
		MemSet( page, 0, sizeof( *page ) );
		page->title = StrDup( pagename );
		page->grid.nPartsX = canvas->current_page->grid.nPartsX;
		page->grid.nPartsY = canvas->current_page->grid.nPartsY;
		AddPage( canvas, page );
		lprintf( "page set to %p", page );
		canvas->current_page = page;
		//SaveButtonConfig( pc_canvas );
	}
}

//---------------------------------------------------------------------------

void RenamePage( PSI_CONTROL pc_canvas )
{
	PCanvasData canvas = GetCanvas( pc_canvas );
	TEXTCHAR pagename[256];
	if( SimpleUserQuery( pagename, sizeof( pagename ), "Enter New Page Name"
							 , pc_canvas ) )
	{
		PPAGE_DATA page = GetPageFromFrame( pc_canvas );
		if( page )
		{
			if( page == canvas->default_page )
			{
				InsertStartupPage( pc_canvas, pagename );
				if( g.flags.multi_edit )
				{
					DisplayFrame( canvas->current_page->frame );
			}
			}
			else
			{
				if( page->title )
					Release( (POINTER)page->title );
				page->title = StrDup( pagename );
			}
		}
		//SaveButtonConfig( pc_canvas );
	}
}

//---------------------------------------------------------------
// PAGE Change Control
//---------------------------------------------------------------------------

static uintptr_t OnCreateMenuButton( PAGE_CHANGER_NAME )( PMENU_BUTTON button )
{
	// add layout, and set title on button...
	// well...
	InterShell_SetButtonText( button, "Change_Page" );
	InterShell_SetButtonStyle( button, "bicolor square" );
	InterShell_SetButtonColors( button, BASE_COLOR_WHITE,BASE_COLOR_GREEN,BASE_COLOR_BLACK, 0 );
	{
		return (uintptr_t)button;//page_changer;
	}
	return 0;
}

static void OnDestroyControl( PAGE_CHANGER_NAME )( uintptr_t psv )
//void CPROC DestroyPageChanger( uintptr_t psv, PMENU_BUTTON button )
{
	// release any private data associated with this button...
	//Release( (PPAGE_DATA)psv );
}

void InterShell_DisableButtonPageChange( PMENU_BUTTON button )
{
	// set a one shot flag to disable the change associated with this button.
	if( button )
		button->flags.bIgnorePageChange = 1;
}

#undef UpdateButtonEx
PUBLIC( void, UpdateButtonEx )( PMENU_BUTTON button, int bEndingEdit  )
{
	UpdateButtonExx( button, bEndingEdit DBG_SRC );
}

static void UpdateControlPositions( PPAGE_DATA page )
{
	PMENU_BUTTON button;
	INDEX idx;
	PCanvasData canvas = (PCanvasData)page->canvas;

	InterShell_DisablePageUpdate( page->frame, TRUE );
	LIST_FORALL( page->controls, idx, PMENU_BUTTON, button )
	{
		PSI_CONTROL pc = QueryGetControl( button );
		if( pc )
			MoveCommon( pc
				, PARTX( button->x ) - page->grid.origin_offset_x
				, PARTY( button->y ) - page->grid.origin_offset_y);
					
	}
	InterShell_DisablePageUpdate( page->frame, FALSE );
}

void SetPageOffset( PPAGE_DATA page, int x, int y )
{
	if( x > page->grid.max_offset_x )
	{
		page->grid.overflow_x += x - page->grid.max_offset_x;
		x = page->grid.max_offset_x;// + log( (double)page->grid.overflow_x );		
	}
	if( y > page->grid.max_offset_y )
	{
		page->grid.overflow_y += y - page->grid.max_offset_y;
		y = page->grid.max_offset_y;// + log( (double)page->grid.overflow_y );		
	}
	if( x < page->grid.min_offset_x )
	{
		page->grid.overflow_x += x - page->grid.min_offset_x;
		x = page->grid.min_offset_x;// - log( (double)page->grid.overflow_x );		
	}
	if( y < page->grid.min_offset_y )
	{
		page->grid.overflow_y += y - page->grid.min_offset_y;
		y = page->grid.min_offset_y;// - log( (double)page->grid.overflow_y );		
	}

	page->grid.origin_offset_x = x;
	page->grid.origin_offset_y = y;

	UpdateControlPositions( page );

}

//--------------------------------------------------------------------------

void SetPageOffsetRelative( PPAGE_DATA page, int x, int y )
{
	SetPageOffset( page, page->grid.origin_offset_x + x, page->grid.origin_offset_y + y );
}

//--------------------------------------------------------------------------

static void MergeButton( PMENU_BUTTON button )
{
	PPAGE_DATA page = button->page;
	PCanvasData canvas = (PCanvasData)page->canvas;
	int real_x = PARTX( button->x );
	int real_y = PARTY( button->y );
	int real_max_x = PARTW( button->x, button->w );
	int real_max_y = PARTH( button->y, button->h );
	int tmp;

	tmp = real_x - (canvas->width - real_max_x/2);
	if( tmp < page->grid.min_offset_x )
		page->grid.min_offset_x = tmp;

	tmp = real_y - (canvas->height - real_max_y/2);
	if( tmp < page->grid.min_offset_y )
		page->grid.min_offset_y = tmp;

	tmp = real_x + (real_max_x/2);
	if( tmp > page->grid.max_offset_x )
		page->grid.max_offset_x = tmp;

	tmp = real_y + (real_max_y/2);
	if( tmp > page->grid.max_offset_y )
		page->grid.max_offset_y = tmp;
}

//--------------------------------------------------------------------------

void UpdatePageRange( PPAGE_DATA page )
{
	PMENU_BUTTON button;
	INDEX idx;
	LIST_FORALL( page->controls, idx, PMENU_BUTTON, button )
	{
		MergeButton( button );
	}
}

//--------------------------------------------------------------------------

void PutButtonOnPage( PPAGE_DATA page, PMENU_BUTTON button )
{
	button->page = page;	
	button->canvas = (PCanvasData)page->canvas;
	AddLink( &page->controls, button );		
	MergeButton( button );
}

//--------------------------------------------------------------------------

static void InvokeThemeAdded( int theme_id )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/theme/add", &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		void (CPROC*f)(int);
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (int) );
		if( f )
			f( theme_id );
	}
}

//--------------------------------------------------------------------------

static void InvokeThemeChanging( void )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/theme/changing", &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		void (CPROC*f)(int);
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (int) );
		if( f )
			f( g.theme_index );
	}
}

//--------------------------------------------------------------------------

static void InvokeThemeChanged( void )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	for( name = GetFirstRegisteredName( TASK_PREFIX "/common/theme/changed", &data );
		name;
		name = GetNextRegisteredName( &data ) )
	{
		void (CPROC*f)(int);
		f = GetRegisteredProcedure2( (CTEXTSTR)data, void, name, (int) );
		if( f )
			f( g.theme_index );
	}
}

//--------------------------------------------------------------------------

void AddTheme( int theme_id )
{
	for( ; g.max_themes <= theme_id; g.max_themes++ )
		InvokeThemeAdded( g.max_themes );
}

//--------------------------------------------------------------------------

void StoreTheme( PCanvasData canvas )
{
	InvokeThemeChanging();

	{
		INDEX idx;
		PPAGE_DATA page;
		LIST_FORALL( canvas->pages, idx, PPAGE_DATA, page )
		{
			SetLink( &page->background_images, g.theme_index, page->background_image );
		}
	}
}

//--------------------------------------------------------------------------

void UpdateTheme( PCanvasData canvas )
{
	if( !g.flags.bPageUpdateDisabled )
		EnableCommonUpdates( canvas->pc_canvas, FALSE );
	{
		INDEX idx;
		PPAGE_DATA page;
		//if( !canvas->pages )
		//	AddLink( &canvas->pages, canvas->default_page );
		LIST_FORALL( canvas->pages, idx, PPAGE_DATA, page )
		{
			INDEX idx2;
			PMENU_BUTTON control;
			CTEXTSTR name;
			CDATA color = (CDATA)(uintptr_t)GetLink( &page->background_colors, g.theme_index );
			if( color )
			{
				page->background_color = color;
			}
			if( name = (CTEXTSTR)GetLink( &page->backgrounds, g.theme_index ) )
			{
				page->background = name;
				page->background_image = (Image)GetLink( &page->background_images, g.theme_index );
			}
			UpdateFontTheme( canvas, g.theme_index );
			LIST_FORALL( page->controls, idx2, PMENU_BUTTON, control )
			{
				PGLARE_SET new_glare = (PGLARE_SET)GetLink( control->glare_set->theme_set, g.theme_index );
				if( new_glare )
				{
					// full flag will cause the control to get reinitialized
					// all parameters will get set.
					control->glare_set = new_glare;
					FixupButton( control );
				}
			}
		}
	}

	{
	}

	InvokeThemeChanged();

	if( !g.flags.bPageUpdateDisabled )
	{
		EnableCommonUpdates( canvas->pc_canvas, TRUE );
		//ReleaseCommonUse( pc_canvas );
		/* disabled auto smudge on enable(TRUE).... so smudge */
		SmudgeCommon( canvas->pc_canvas );
	}
}

//--------------------------------------------------------------------------

void InterShell_SetPageColor( PPAGE_DATA page, CDATA color )
{
	if( page )
	{
		page->background_color = color;
		SetLink( &page->background_colors, local_page_info.current_page_theme, (POINTER)(uintptr_t)color );
		SmudgeCommon( page->frame );
	}
}

//--------------------------------------------------------------------------


INTERSHELL_NAMESPACE_END

