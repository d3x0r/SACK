
#define _INCLUDE_CLIPBOARD
#include <stdhdrs.h>
#include <sharemem.h>
#include <logging.h>

#include "controlstruc.h"
#include <controls.h>
#include <keybrd.h>
#include <psi.h>

#include "ctllistbox.h"

struct combobox {
	_32 data;
	LOGICAL first_item;
	LOGICAL first_show;
	PRENDERER popup_renderer;
	void (CPROC*OnPopup)( PTRSZVAL psvPopup, LOGICAL show );
	PTRSZVAL psvPopup;
	PSI_CONTROL self;
	PSI_CONTROL edit;
	PSI_CONTROL expand_button;
	_32 popup_width, popup_height;
	PSI_CONTROL popup_frame;
	PSI_CONTROL popup_frame_listbox;
	SelectionChanged OnSelect;
	PTRSZVAL psvOnSelect;
};
typedef struct combobox COMBOBOX, *PCOMBOBOX;

static void CPROC HandleLoseFocus( PTRSZVAL dwUser, PRENDERER pGain )
{
	PCOMBOBOX pcbx = (PCOMBOBOX)dwUser;
	if( pGain != pcbx->popup_renderer )
	{
		HideControl( pcbx->popup_frame );
	}
}

static void CPROC ExpandButtonEvent(PTRSZVAL psvCbx, PSI_CONTROL pc )
{
	PCOMBOBOX pcbx = (PCOMBOBOX)psvCbx;
	S_32 x = 0, y = 0;
	SizeFrame( pcbx->popup_frame, pcbx->popup_width, pcbx->popup_height );
	SizeControl( pcbx->popup_frame_listbox, pcbx->popup_width, pcbx->popup_height );
	GetPhysicalCoordinate( pcbx->self, &x, &y, FALSE );
	MoveFrame( pcbx->popup_frame, x, y );
	lprintf( "Send popup event before show..." );
	if( pcbx->OnPopup )
		pcbx->OnPopup( pcbx->psvPopup, TRUE );
	lprintf( "Sent popup event before show..." );
	DisplayFrameOver( pcbx->popup_frame, pc );
	if( !pcbx->first_show )
	{
		pcbx->popup_renderer = GetFrameRenderer( pcbx->popup_frame );
		SetLoseFocusHandler( pcbx->popup_renderer, HandleLoseFocus, (PTRSZVAL)pcbx );
		pcbx->first_show = TRUE;
	}
	//OwnMouse( GetFrameRenderer( pcbx->popup_frame ), TRUE );
	//CaptureCommonMouse( pcbx->popup_frame, TRUE );
	//CommonWait( pcbx->popup_frame );
}

static void PopupSelected( PTRSZVAL psvCbx, PSI_CONTROL pc, PLISTITEM hli )
{
	PCOMBOBOX pcbx = (PCOMBOBOX)psvCbx;
	TEXTCHAR buf[256];
	//OwnMouse( GetFrameRenderer( pc ), FALSE );
	HideControl( pcbx->popup_frame );
	GetItemText( hli, 256, buf );
	SetControlText( pcbx->edit, (CTEXTSTR)buf );
	
	if( pcbx->OnSelect )
		pcbx->OnSelect( pcbx->psvOnSelect, pcbx->self, hli );

	lprintf( "Send popup event before hide..." );
	if( pcbx->OnPopup )
		pcbx->OnPopup( pcbx->psvPopup, FALSE );
	lprintf( "Sent popup event after hide..." );
}

static void ExpandButtonDraw( PTRSZVAL psv, PSI_CONTROL pc)
{
	Image surface = GetControlSurface( pc );
	int n;
	int y = ( surface->height / 2 ) - 3;
	CDATA c = GetControlColor( pc, SHADOW );
	for( n = 0; n < 7; n++ )
	{
		do_hline( surface, y + n, (surface->width/2) - ( 6-n ), (surface->width/2) + ( 6-n ) + 1, c );
	}
}

static int OnCreateCommon( WIDE("Combo Box") )( PSI_CONTROL pc )
{
	ValidatedControlData( PCOMBOBOX, COMBOBOX_CONTROL, pcbx, pc );
	Image surface = GetControlSurface( pc );
	pcbx->self = pc;
	pcbx->first_item = TRUE;
	pcbx->expand_button = MakeNamedControl( pc, NORMAL_BUTTON_NAME, surface->width - surface->height, 0, surface->height, surface->height, 0 );
	SetButtonDrawMethod( pcbx->expand_button, ExpandButtonDraw, 0 );
	SetButtonPushMethod( pcbx->expand_button, ExpandButtonEvent, (PTRSZVAL)pcbx );
	pcbx->edit = MakeNamedControl( pc, EDIT_FIELD_NAME, 0, 0, surface->width - surface->height, surface->height, 0 );
	SetEditControlReadOnly( pcbx->edit, TRUE );
	pcbx->popup_height = surface->width;
	pcbx->popup_width = surface->width;

	pcbx->popup_frame = CreateFrame( NULL, 0, 0, surface->width, surface->width, BORDER_THIN|BORDER_NOCAPTION, NULL );
	pcbx->popup_frame_listbox = MakeNamedControl( pcbx->popup_frame, LISTBOX_CONTROL_NAME, 0, 0, surface->width, surface->height, 0 );
	SetSelChangeHandler( pcbx->popup_frame_listbox, PopupSelected, (PTRSZVAL)pcbx );
	//ValidatedControlData( PCOMBOBOX, COMBOBOX_CONTROL, pcbx, pc );
	return TRUE;
}

static void OnSizeCommon( WIDE( "Combo Box" ) )( PSI_CONTROL pc, LOGICAL start )
{
	ValidatedControlData( PCOMBOBOX, COMBOBOX_CONTROL, pcbx, pc );
	if( !start && pcbx )
	{
		Image surface = GetControlSurface( pc );
		MoveSizeCommon( pcbx->expand_button, surface->width - surface->height, 0, surface->height, surface->height );
		MoveSizeCommon( pcbx->edit, 0, 0, surface->width - surface->height, surface->height );

	}
}

void SetComboBoxPopupSize( PSI_CONTROL pc, _32 w, _32 h )
{
	ValidatedControlData( PCOMBOBOX, COMBOBOX_CONTROL, pcbx, pc );
	if( pcbx )
	{
		pcbx->popup_width = w;
		pcbx->popup_height = h;
	}
}

void ResetComboBox( PSI_CONTROL pc )
{
	ValidatedControlData( PCOMBOBOX, COMBOBOX_CONTROL, pcbx, pc );
	if( pcbx )
	{
		pcbx->first_item = TRUE;
		ResetList( pcbx->popup_frame_listbox );
		SetControlText( pcbx->edit, NULL );
	}
}


static int OnDrawCommon( WIDE("Combo Box") )( PSI_CONTROL pc )
{
	//ValidatedControlData( PCOMBOBOX, COMBOBOX_CONTROL, pcbx, pc );
	return TRUE;
}


CONTROL_REGISTRATION
combobox_control = { COMBOBOX_CONTROL_NAME
					, { {173, 24}, sizeof( COMBOBOX ), BORDER_INVERT_THIN }
};

PRIORITY_PRELOAD( RegisterComboBox, PSI_PRELOAD_PRIORITY )
{
	DoRegisterControl( &combobox_control );
	//RegisterAlias( PSI_ROOT_REGISTRY WIDE("/control/") EDIT_FIELD_NAME "/rtti", WIDE("psi/control/combobox/rtti") );
}


PLISTITEM AddComboBoxItem( PSI_CONTROL pc, CTEXTSTR text )
{
	ValidatedControlData( PCOMBOBOX, COMBOBOX_CONTROL, pcbx, pc );
	if( pcbx )
	{
		PLISTITEM pli;
		pli = AddListItem( pcbx->popup_frame_listbox, text );
		if( pcbx->first_item )
		{
			pcbx->first_item = FALSE;
			SetControlText( pcbx->edit, text );
			if( pcbx->OnSelect )
				pcbx->OnSelect( pcbx->psvOnSelect, pc, pli );
		}
		return pli;
	}
	return NULL;
}

void SetComboBoxSelChangeHandler( PSI_CONTROL pc, SelectionChanged proc, PTRSZVAL psvUser )
{
	ValidatedControlData( PCOMBOBOX, COMBOBOX_CONTROL, pcbx, pc );
	if( pcbx )
	{
		pcbx->OnSelect = proc;
		pcbx->psvOnSelect = psvUser;
	}
}


void SetComboBoxPopupEventCallback( PSI_CONTROL pc, void (CPROC*PopupEvent)( PTRSZVAL,LOGICAL ), PTRSZVAL psvEvent )
{
	ValidatedControlData( PCOMBOBOX, COMBOBOX_CONTROL, pcbx, pc );
	if( pcbx )
	{
		pcbx->OnPopup = PopupEvent;
		pcbx->psvPopup = psvEvent;
	}
}

void SetComboBoxSelectedItem( PSI_CONTROL pc, PLISTITEM hli )
{
	ValidatedControlData( PCOMBOBOX, COMBOBOX_CONTROL, pcbx, pc );
	if( pcbx )
	{
		SetSelectedItem( pcbx->popup_frame_listbox, hli );
		SetControlText( pcbx->edit, hli->text );
		if( pcbx->OnSelect )
			pcbx->OnSelect( pcbx->psvOnSelect, pc, hli );
	}
}

