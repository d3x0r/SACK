#include <stdhdrs.h>
#include <sharemem.h>
#include "controlstruc.h"
#include <psi.h>

PSI_SHEETS_NAMESPACE
//#define DEBUG_SHEET

typedef struct sheet_tag SHEET, *PSHEET;
struct sheet_tag {
	struct {
		uint32_t bDisabled : 1;
		uint32_t bCustomColors : 1;
	} flags;
	PSI_CONTROL content;  // each content has an ID... so select by ID;
	Image active_slices[3], inactive_slices[3];
	Image active, inactive;
	CDATA cActiveText, cInactiveText;
	uint32_t tab_width;
	uint32_t tab_pad;
	struct sheet_tag *next;struct sheet_tag **me;
	//DeclareLink( struct sheet_tag );
};

typedef struct multi_sheet_tag {
	struct {
		uint32_t bNoTabs : 1;
		uint32_t bCustomColors : 1;
	} flags;
	PSI_CONTROL mount_point;
	PSHEET sheets;
	PSHEET current;
	PSHEET prior_sheet;
	PSHEET first;
	Image active_slices[3], inactive_slices[3];
	Image active, inactive;
	CDATA cActiveText, cInactiveText;

	uint32_t first_offset; // the last may be the whole increment... so offset the first.
	uint32_t _b;
	uint32_t height;
	uint32_t tab_pad; // this needs to be like some portion of the image..
} SHEETCONTROL, *PSHEETCONTROL;

static void ComputeHeight( PSI_CONTROL pc )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pc );
	uint32_t TAB_HEIGHT, tmp1 = 0, tmp2 = 0;
	SFTFont font = GetFrameFont( pc );
	GetStringSizeFont( WIDE(" "), NULL, &TAB_HEIGHT, font );
	TAB_HEIGHT += 4;
	if( psc->active )
		tmp1 = psc->active->height;
	if( psc->inactive )
		tmp2 = psc->inactive->height;
	if( tmp1 > TAB_HEIGHT )
	{
		if( tmp1 > tmp2 )
		{
			lprintf( WIDE("Settin tab height to %")_32f WIDE(""), tmp1 );
			TAB_HEIGHT = tmp1;
		}
		else
		{
			lprintf( WIDE("Settin tab height tox %")_32f WIDE(""), tmp2 );
			TAB_HEIGHT = tmp2;
		}
	}
	else if( tmp2 > TAB_HEIGHT )
	{
		lprintf( WIDE("Settin tab height to %")_32fs WIDE(""), tmp2 );
		TAB_HEIGHT = tmp2;
	}

	{
		// need to go through all the sheets
		// and check their tab image sizes also...
	}
	if( psc->active )
		if( USS_LT( TAB_HEIGHT, uint32_t, psc->active->height, int ) )
			TAB_HEIGHT = psc->active->height;
	if( psc->height != TAB_HEIGHT )
	{
		psc->height = TAB_HEIGHT;
#ifdef DEBUG_SHEET
		lprintf( WIDE("Moving the frame? to %d,%d"), 0, psc->height );
#endif
		MoveSizeFrame( psc->mount_point, 0, psc->height
						 , pc->surface_rect.width
						 , pc->surface_rect.height - psc->height );
	}
}

static void BlotTab( Image surface, Image slices[3], int x, int width, int height )
{
	uint32_t w1, w2;
	w1 = slices[0]->width;
	w2 = slices[2]->width;
	if( SUS_LT( width, int, (w1+w2), uint32_t ) )
	{
		BlotScaledImageSizedToAlpha( surface, slices[0]
											, x, 0
											, width / 2, height
											, ALPHA_TRANSPARENT );
		// computation of width - (width/2) accounts for the rounding
		// error if width is odd...
		BlotScaledImageSizedToAlpha( surface, slices[2]
											, x + (width/2), 0
											, width - (width / 2), height
											, ALPHA_TRANSPARENT );
	}
	else
	{
		BlotScaledImageSizedToAlpha( surface, slices[0]
											, x, 0
											, w1, height
											, ALPHA_TRANSPARENT );
		//BlotImageAlpha( surface, slices[0], x, 0, ALPHA_TRANSPARENT );

		BlotScaledImageSizedToAlpha( surface, slices[2]
											, x + (width - w2), 0
											, w2, height
											, ALPHA_TRANSPARENT );
		//BlotImageAlpha( surface, slices[2]
		//				  , x + (width - w2), 0
		//				  , ALPHA_TRANSPARENT );
	// just scale center portion to width of tab
		if( USS_LT( ( w1+w2 ),uint32_t, width,int) ) // if equal or less, no reason to do this...
			BlotScaledImageSizedToAlpha( surface, slices[1]
												, x + w1
												, 0
												, width - (w1+w2)
												, height
												, ALPHA_TRANSPARENT );
	}
}

static int CPROC DrawSheetControl( PSI_CONTROL pc )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pc );
	Image surface;
	PSHEET sheet;
	SFTFont font;
	if( !psc )
	{
		lprintf( WIDE("Control to draw is not a sheet control."));
		return 0;
	}
	//lprintf( WIDE("control surface is %p but we don't know the bit pointer")
	//		 , pc->Surface );
	surface = pc->Surface;
	// need to compute how big the tab is for the caption on the dialog... and
	// whether we need additionally some buttons present...
	//lprintf( WIDE("So yeah we clear the sufrace...") );
	//ClearImageTo( surface, basecolor(pc)[NORMAL] );
	//lprintf( WIDE("Get a font") );
	font = GetFrameFont( pc );
	{
		int x = -((int)psc->first_offset);
		sheet = psc->sheets;
		psc->first = sheet;
		ComputeHeight( pc );
		while( sheet )
		{
			uint32_t tab_width;
			uint32_t TAB_HEIGHT;

			GetStringSizeFont( GetText( sheet->content->caption.text)
								  , &sheet->tab_width, &TAB_HEIGHT, font );
			//lprintf( WIDE("x : %d next: %d"), x, sheet->tab_width );
			if( sheet == psc->current )
			{
				//lprintf( WIDE("Surface height and my hight %d %d"), surface->height, psc->height );
				if( sheet->active )
				{
					//sheet->tab_width += sheet->tab_pad; // 4 left, 4 right... good pad even for smallness
					BlotTab( surface, sheet->active_slices, x
							 , tab_width = sheet->tab_width + sheet->tab_pad, psc->height );
				}
				else if( psc->active )
				{
					//sheet->tab_width += psc->tab_pad; // 4 left, 4 right... good pad even for smallness
					BlotTab( surface, psc->active_slices, x
							 , tab_width = sheet->tab_width + psc->tab_pad, psc->height );
				}
				else
				{
					tab_width = sheet->tab_width + psc->tab_pad;
					BlatColor( surface, x, 0, tab_width, psc->height, basecolor(pc)[NORMAL] );
					do_vline( surface, x, 0, psc->height, basecolor(pc)[SHADE] );
					do_vline( surface, x+1, 0, psc->height, basecolor(pc)[HIGHLIGHT] );
					do_vline( surface, x + tab_width-1, 0, psc->height, basecolor(pc)[SHADE] );
					do_vline( surface, x + tab_width, 0, psc->height, basecolor(pc)[SHADOW] );
					do_hline( surface, 0, x+1, x + tab_width, basecolor(pc)[HIGHLIGHT] );
				}
				if( sheet->content->caption.text )
				{
					CDATA color;
					if( sheet->flags.bCustomColors )
						color = sheet->cActiveText;
					else if( psc->flags.bCustomColors )
						color = psc->cActiveText;
					else
						color = basecolor(pc)[TEXTCOLOR];
					//lprintf( WIDE("Putting string out at %d,%d %s"), x + 4, 3, GetText( sheet->content->caption.text ) );
					PutStringFont( surface, x + ( tab_width - sheet->tab_width) / 2, (psc->height - TAB_HEIGHT)/2, color, 0
									 , GetText( sheet->content->caption.text), font );
				}
				//else
				//	lprintf( WIDE("no content caption text") );
			}
			else
			{
				if( sheet->inactive )
				{
					BlotTab( surface, sheet->inactive_slices, x
							 , tab_width = sheet->tab_width + sheet->tab_pad, psc->height );
				}
				else if( psc->inactive )
				{
					BlotTab( surface, psc->inactive_slices, x
							 , tab_width = sheet->tab_width + psc->tab_pad, psc->height );
				}
				else
				{
					tab_width = sheet->tab_width + psc->tab_pad;
				//lprintf( WIDE("Surface height and my hight %d %d"), surface->height, psc->height );
					BlatColor( surface, x, 0, tab_width, psc->height, basecolor(pc)[NORMAL] );
					do_vline( surface, x, 0, psc->height, basecolor(pc)[SHADE] );
					do_vline( surface, x+1, 0, psc->height, basecolor(pc)[SHADOW] );
					do_vline( surface, x + tab_width-1, 0, psc->height, basecolor(pc)[SHADE] );
					do_vline( surface, x + tab_width, 0, psc->height, basecolor(pc)[HIGHLIGHT] );
					do_hline( surface, 0, x+1, x + tab_width, basecolor(pc)[SHADE] );
				}
				if( sheet->content->caption.text )
				{
					CDATA color;
					if( sheet->flags.bCustomColors )
						color = sheet->cInactiveText;
					else if( psc->flags.bCustomColors )
						color = psc->cInactiveText;
					else
						color = basecolor(pc)[SHADE];
					//lprintf( WIDE("Putting string out at %d,%d %s"), x + 4, 3, GetText( sheet->content->caption.text ) );
					PutStringFont( surface, x + (tab_width-sheet->tab_width)/2, (psc->height - TAB_HEIGHT)/2, color, 0
									 , GetText( sheet->content->caption.text), font );
				}
				//else
				//	lprintf( WIDE("no content caption text") );
			}
			sheet->tab_width = tab_width;
			x += sheet->tab_width;// sheet->tab_width;
			sheet = sheet->next;
		}
	}
	return 1;
}

static PSHEET GetSheetID( PSHEETCONTROL psc, uint32_t ID )
{
	PSHEET sheet = psc->sheets;
	while( sheet )
	{
		if( !sheet->content )
		{
			Log( WIDE("Sheet exists without content!?!?!?") );
			continue;
		}
		if( sheet->content->nID == ID )
		{
			return sheet;
		}
		sheet = sheet->next;
	}
	return NULL;
}

static void SetCurrentPage( PSI_CONTROL pControl, PSHEET sheet )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pControl );
	//lprintf( WIDE("Set a current page?") );
	if( sheet != psc->current )
	{
		// revert to prior sheet... if there was one?
		// what about the times I want to set NO page
		// guess I ahve to clear psc->prior_sheet
		if( !sheet &&
			 psc->prior_sheet &&
			!psc->prior_sheet->flags.bDisabled )
			sheet = psc->prior_sheet;

		if( psc->current &&
			!psc->current->flags.bDisabled )
			psc->prior_sheet = psc->current;
		if( psc->current )
		{
#ifdef DEBUG_SHEET
			lprintf( WIDE("Orphan control %p"), psc->current->content );
#endif
			OrphanCommonEx( (PSI_CONTROL)psc->current->content, FALSE );
			if( !sheet )
				SmudgeCommon( pControl );
		}

		psc->current = sheet;
		if( sheet )
		{
#ifdef DEBUG_SHEET
			lprintf( WIDE("Adopt control into %p (%p) %p")
					 , psc->mount_point
					 , psc->mount_point->child
					 , sheet->content );
#endif
			AdoptCommonEx( (PSI_CONTROL)psc->mount_point, NULL, (PSI_CONTROL)sheet->content, FALSE );
			SmudgeCommon( pControl );
		}
	}
	else
	{
		// current is already current - that's good.
	}
}

static int CPROC MouseSheetControl( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t b )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pc );
	PSHEET sheet;
	int check_x;
	if( b == MK_NO_BUTTON )
	{
		psc->_b = 0;
		return 0;
	}
	check_x = psc->first_offset;
	sheet = psc->first;
	//lprintf( WIDE("mouse: %d,%d "), x, y );
	if( y >= 0 && SUS_LTE( y, int32_t, psc->height, uint32_t ) )
	{
		while( sheet )
		{
			//lprintf( WIDE("Checking %d vs %d, %d"), x, check_x, check_x + sheet->tab_width );
			if( !sheet->flags.bDisabled )
			{
				//lprintf( WIDE("Okay it's not disabled...") );
				if( x > check_x && SUS_LT( x, int32_t, ( check_x + sheet->tab_width ), uint32_t ) )
				{
					if( MAKE_NEWBUTTON( b, psc->_b ) & MK_LBUTTON )
					{
						SetCurrentPage( pc, sheet );
						break;
					}
				}
			}
			//else
			//	lprintf( WIDE("Disabled!") );
			check_x += sheet->tab_width;
			sheet = sheet->next;
		}
	}
	return 1;
}

static void CPROC DestroySheetControl( PSI_CONTROL pc )
{
	//ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pc );
}

static void OnEditFrame( SHEET_CONTROL_NAME )(PSI_CONTROL pc )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pc );
	// my frame is having it's edit mode enabled.
	psc->prior_sheet = NULL;
	SetCurrentPage( pc, NULL );
	{
		PSHEET sheet;
		for( sheet = psc->sheets; sheet; sheet = sheet->next )
		{
			EditFrame( sheet->content, TRUE );
		}
	}
}

static void OnEditFrameDone( SHEET_CONTROL_NAME )( PSI_CONTROL pc )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pc );
	{
		PSHEET sheet;
		for( sheet = psc->sheets; sheet; sheet = sheet->next )
		{
			EditFrame( sheet->content, FALSE );
		}
	}
	SetCurrentPage( pc, psc->prior_sheet );

}

static void CPROC AddingSheet( PSI_CONTROL sheet, PSI_CONTROL page )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, sheet );
	// if we haven't gotten a mount point yet, no use in
	// doing anything here.
	if( !psc->mount_point || sheet == psc->mount_point )
	{
		// this is within the tabs, and serves as
		// a container for child drawing...
		// don't remount our mount point...
		return;
	}
	SetCommonBorder( page, BORDER_NONE|BORDER_NOCAPTION );
#ifdef DEBUG_SHEET
	lprintf( WIDE("Inserting a control into the sheet control... promoting to a sheet."));
#endif
	OrphanCommonEx( page, FALSE );
	AddSheet( sheet, page );
	//(PSI_CONTROL)sheet->common.parent = NULL;
}

static int CPROC InitSheetControl( PSI_CONTROL pc )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pc );
	if( psc )
	{
		FRACTION f;
		//uint32_t w, h;
		psc->mount_point = MakePrivateControl( pc, CONTROL_FRAME
														 , 0, 0 // starts with no height, as tabs are added, and measured, this is resized.
														 , pc->rect.width
														 , pc->rect.height - psc->height
														 , 0 );
		SetFraction( f, 1, 1 );
		SetCommonScale( psc->mount_point, &f, &f );
		psc->tab_pad = 8; // default font pad
		SetCommonBorder( psc->mount_point, BORDER_WITHIN|BORDER_THINNER|BORDER_INVERT );
		//pc->AddedControl = AddingSheet;
		ComputeHeight( pc );
		return TRUE;
	}
	return FALSE;
}

CONTROL_REGISTRATION
sheet_control = { SHEET_CONTROL_NAME
					 , { {240, 180}, sizeof( SHEETCONTROL ), BORDER_NONE }
					 , InitSheetControl// init
					 , NULL //load
					 , DrawSheetControl
					 , MouseSheetControl
					 , NULL
					 , DestroySheetControl
					 , NULL// getpage
					 , NULL// apply
					 , NULL // save
					 , AddingSheet
};
PRIORITY_PRELOAD( register_sheet_control, PSI_PRELOAD_PRIORITY ) {
	DoRegisterControl( &sheet_control );
}


PSI_PROC( void, AddSheet )( PSI_CONTROL pControl, PSI_CONTROL contents )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pControl );
	PSHEET sheet;
	if( !psc )
	{
		Log( WIDE("Not adding to a sheet...") );
		return;
	}
	if( !contents )
	{
		Log( WIDE("Can't add no content") );
		return;
	}
	sheet = (PSHEET)Allocate( sizeof( SHEET ) );
	MemSet( sheet, 0, sizeof( SHEET ) );
	SetCommonBorder( contents, BORDER_NOCAPTION | BORDER_NONE );
	sheet->content = contents;
	//sheet->next = NULL;
	//sheet->me = NULL;
	if( sheet->content->caption.text )
	{
		SFTFont font = GetCommonFont( pControl );
		sheet->tab_width = GetStringSizeFont( GetText( sheet->content->caption.text ), NULL, NULL, font );
#ifdef DEBUG_SHEET
		Log1( WIDE("Tab width is %d"), sheet->tab_width );
#endif
	}
	else
	{
		Log( WIDE("Sheet has no caption...") );
		sheet->tab_width = 0;

	}
	sheet->tab_width += 8; // 2 left, 2 right, 2 padd each side away from text?

	// can't use standard 'link thing' cause this links at end of list.
	if( psc->sheets )
	{
		PSHEET next = psc->sheets;
		while( next->next )
		{
			if( next->content->nID == sheet->content->nID )
			{
				Log( WIDE("Warning - sheet with ID already exists...") );
				Log( WIDE("Will not be able to select this sheet externally...") );
			}
			next = next->next;
		}
		next->next = sheet;
		sheet->me = &next->next;
	}
	else
	{
		//lprintf( WIDE("Adopting first sheet...") );
		psc->sheets = sheet;
		sheet->me = &psc->sheets;
		SetCurrentSheet( pControl, sheet->content->nID );
	}
}

PSI_PROC( int, RemoveSheet )( PSI_CONTROL pControl, uint32_t ID )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pControl );
	if( psc )
	{
		PSHEET sheet = psc->sheets;
		while( sheet )
		{
			if( !sheet->content )
			{
				Log( WIDE("Sheet exists without content!?!?!?") );
				continue;
			}
			if( sheet->content->nID == ID )
			{
				// keep the content - the application has the reponsibility
				// of freeing the controls.
				//DestroyCommon( &sheet->content );
				Release( sheet );
				UnlinkThing( sheet );
				return TRUE;
			}
			sheet = sheet->next;
		}
	}
	return FALSE;
}

PSI_PROC( void, SetCurrentSheet )( PSI_CONTROL pControl, uint32_t ID )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pControl );
	if( psc )
	{
		PSHEET sheet = GetSheetID( psc, ID );
		if( sheet )
			SetCurrentPage( pControl, sheet );
	}
}


PSI_PROC( int, GetSheetSize )( PSI_CONTROL pControl, uint32_t *width, uint32_t *height )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pControl );
	if( psc )
	{
		if( width )
			*width = psc->mount_point->surface_rect.width;
		if( height )
			*height = psc->mount_point->surface_rect.height;
		return 1;
	}
	return 0;
}

PSI_PROC( PSI_CONTROL, GetSheet )( PSI_CONTROL pControl, uint32_t ID )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pControl );
	if( psc )
	{
		PSHEET sheet = GetSheetID( psc, ID );
		if( sheet )
			return (PSI_CONTROL)sheet->content;
	}
	return NULL;
}

PSI_PROC( PSI_CONTROL, GetSheetControl )( PSI_CONTROL pControl
												 , uint32_t IDSheet
												 , uint32_t IDControl )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pControl );
	if( psc )
	{
		PSI_CONTROL sheet = GetSheet( pControl, IDSheet );
		return GetControl( sheet, IDControl );
	}
	return NULL;
}

PSI_PROC( PSI_CONTROL, GetCurrentSheet )( PSI_CONTROL pControl )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pControl );
	if( psc )
	{
		if( psc->current )
			return psc->current->content;
	}
	return NULL;
}


PSI_PROC( void, DisableSheet )( PSI_CONTROL pControl, uint32_t ID, LOGICAL bDisable )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pControl );
	if( psc )
	{
		PSHEET sheet = GetSheetID( psc, ID );
		if( sheet )
		{
			sheet->flags.bDisabled = bDisable;
			if( psc->current == sheet )
			{
				PSHEET check;
				for( check = psc->sheets; check; check = check->next )
					if( check == psc->prior_sheet )
					{
						SetCurrentPage( pControl, check );
						break;
					}
				if( !check )
				{
					SetCurrentPage( pControl, NULL );
				}
			}
		}
	}
}

//-------------------------------------------------------------------

static void SliceImages( Image image, Image slices[3] )
{
	int n;
	for( n = 0; n < 3; n++ )
		if( slices[n] )
		{
			UnmakeImageFile( slices[n] );
			slices[n] = NULL;
		}
	if( image )
	{
		uint32_t w = image->width;
		uint32_t mid;
		if( w & 1 )
		{
			w /= 2;
			mid = 1;
		}
		else
		{
			w = (w-1) / 2;
			mid = 2;
		}
		slices[0] = MakeSubImage( image, 0, 0, w, image->height );
		slices[1] = MakeSubImage( image, w, 0, mid, image->height );
		slices[2] = MakeSubImage( image, w+mid, 0, w, image->height );
	}
}

//-------------------------------------------------------------------

void SetTabImages( PSI_CONTROL pControl, Image active, Image inactive )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pControl );
	if( psc )
	{
		psc->active = active;
		psc->inactive = inactive;
		if( active )
			psc->tab_pad = active->width / 2;
		SliceImages( psc->active
					  , psc->active_slices );
		SliceImages( psc->inactive
					  , psc->inactive_slices );
		ComputeHeight( pControl );
		SmudgeCommon( pControl );
	}
}

//-------------------------------------------------------------------

// set tab images on a per-sheet basis, overriding the defaults specified.
void SetSheetTabImages( PSI_CONTROL pControl, uint32_t ID, Image active, Image inactive )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pControl );
	if( psc )
	{
		PSHEET sheet = GetSheetID( psc, ID );
		if( sheet )
		{
			sheet->active = active;
			sheet->inactive = inactive;
			sheet->tab_pad = active->width / 2;
			SliceImages( sheet->active
						  , sheet->active_slices );
			SliceImages( sheet->inactive
						  , sheet->inactive_slices );
			ComputeHeight( pControl );
			SmudgeCommon( pControl );
		}
	}
}

//-------------------------------------------------------------------

void SetTabTextColors( PSI_CONTROL pControl, CDATA cActive, CDATA cInactive )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pControl );
	if( psc )
	{
		psc->flags.bCustomColors = 1;
		psc->cActiveText = cActive;
		psc->cInactiveText = cInactive;
		SmudgeCommon( pControl );
	}
}

//-------------------------------------------------------------------

void SetSheetTabTextColors( PSI_CONTROL pControl, uint32_t ID, CDATA cActive, CDATA cInactive )
{
	ValidatedControlData( PSHEETCONTROL, SHEET_CONTROL, psc, pControl );
	if( psc )
	{
		PSHEET sheet = GetSheetID( psc, ID );
		if( sheet )
		{
			sheet->flags.bCustomColors = 1;
			sheet->cActiveText = cActive;
			sheet->cInactiveText = cInactive;
			SmudgeCommon( pControl );
		}
	}
}

PSI_SHEETS_NAMESPACE_END

//-------------------------------------------------------------------
// $Log: ctlsheet.c,v $
// Revision 1.30  2005/03/21 20:41:35  panther
// Protect against super large fonts, remove edit frame from palette, and clean up some warnings.
//
// Revision 1.29  2005/03/07 00:03:04  panther
// Reformatting, removed a lot of superfluous logging statements.
//
// Revision 1.28  2005/03/04 19:07:32  panther
// Define SetItemText
//
// Revision 1.27  2005/02/18 19:42:38  panther
// fix some update issues with hiding and revealing controls/frames... minor fixes for new API changes
//
// Revision 1.26  2005/02/01 02:20:23  panther
// Debugging added...
//
// Revision 1.25  2005/01/10 21:43:42  panther
// Unix-centralize makefiles, also modify set container handling of getmember index
//
// Revision 1.24  2004/12/20 19:41:00  panther
// Minor reformats and function protection changes (static)
//
// Revision 1.23  2004/12/05 00:22:44  panther
// Fix focus flag reference and edit controls begin to work.  Sheet controls are still flaky
//
// Revision 1.22  2004/11/29 11:29:53  panther
// Minor code cleanups, investigate incompatible display driver
//
// Revision 1.21  2004/10/24 20:09:47  d3x0r
// Sync to psilib2... stable enough to call it mainstream.
//
// Revision 1.7  2004/10/12 08:10:51  d3x0r
// checkpoint... frames are controls, and anything can be displayed...
//
// Revision 1.6  2004/10/08 01:21:17  d3x0r
// checkpoint
//
// Revision 1.5  2004/10/06 09:52:16  d3x0r
// checkpoint... total conversion... now how does it work?
//
// Revision 1.4  2004/10/05 00:20:29  d3x0r
// Break out these rather meaty parts from controls.c
//
// Revision 1.3  2004/09/28 16:52:18  d3x0r
// compiles - might work... prolly not.
//
// Revision 1.2  2004/09/27 20:44:28  d3x0r
// Sweeping changes only a couple modules left...
//
// Revision 1.1  2004/09/19 19:22:31  d3x0r
// Begin version 2 psilib...
//
// Revision 1.15  2004/09/03 14:43:48  d3x0r
// flexible frame reactions to font changes...
//
// Revision 1.14  2004/08/24 17:18:00  d3x0r
// Fix last couple c files for new control_proc_def macro
//
// Revision 1.13  2004/06/16 03:02:50  d3x0r
// checkpoint
//
// Revision 1.12  2003/12/03 11:06:42  panther
// Added extended draw options to orphan, adopt. Fixed sheet control issues
//
// Revision 1.11  2003/12/03 10:44:48  panther
// Implement OrphanEx, still have probs with sheets
//
//
