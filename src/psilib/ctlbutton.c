#include <stdhdrs.h>

#include "controlstruc.h"
#include <keybrd.h>

#include <psi.h>

PSI_BUTTON_NAMESPACE

#define PutMenuString PutString
//------------------------------------------------------------------------------

typedef struct local_tag
{
	struct {
		_32 bInited : 1;
		_32 bTouchDisplay : 1;
	} flags;
} LOCAL;

static LOCAL l;
//------------------------------------------------------------------------------

typedef struct button {
	CTEXTSTR ClickMethodName;
	void (CPROC*ClickMethod)( PTRSZVAL, PSI_CONTROL );
	PTRSZVAL ClickData;
	_32 attr;
	struct {
		_32 pressed:1;
	}buttonflags;
	CDATA color;
	_32 centx; // offset from center to draw...
	S_32 topy; // top of the caption...
	_32 _b; // prior button states...
	Image pImage;
	CTEXTSTR DrawMethodName;
	void (CPROC*DrawMethod)( PTRSZVAL, PSI_CONTROL );
	PTRSZVAL DrawData;
} BUTTON, *PBUTTON;


typedef struct {
	struct {
		_32 bChecked:1;
		_32 pressed:1;
		_32 bCallChecked : 1;
		_32 bCallUnchecked : 1;
		_32 bCallAll : 1;
	} flags;
	int centx;
	int groupid; // if is not zero - will work as a radio button
	CTEXTSTR ClickMethodName;
	void (CPROC*ClickMethod)( PTRSZVAL, PCONTROL );
	PTRSZVAL ClickData;
	Image pCheckWindow;
	Image pCheckSurface;
	int _b;
} CHECK, *PCHECK;

//---------------------------------------------------------------------------

static int Init( void )
{
	if( !l.flags.bInited )
	{
		l.flags.bTouchDisplay = IsTouchDisplay();
		l.flags.bInited = TRUE;
	}
	return TRUE;
}


//---------------------------------------------------------------------------

	void DrawButtonCaption( PSI_CONTROL pc, PBUTTON pb, int xofs, int yofs, CDATA color, _32 *yout, _32 *maxwout, SFTFont font )
{
	_32 y = 0;
	_32 w, h, maxw = 0;
	TEXTCHAR *start = GetText( pc->caption.text ), *end;
	//lprintf( WIDE("Drawing button caption: %s"), start );
	GetStringSizeFontEx( start, 1, NULL, &h, font );
	while( start )
	{
		end = strchr( start, '\n' );
		if( !end )
			end = start + strlen(start);
		if( end[0] )
			start = end+1;
		else
			start = NULL;
		y += h;
	}
	y = (pc->surface_rect.height - y)/2;
	start = GetText( pc->caption.text );
	while( start )
	{
		end = strchr( start, '\n' );
		if( !end )
		{
			end = start + strlen(start);
		}
		w = GetStringSizeFontEx( start, end-start, NULL, NULL, font );
		if( w > maxw )
			maxw = w;
		PutStringFontEx( pc->Surface
							, xofs - w/2, y + yofs
							, color, 0
							, start, end-start
							, font );
		if( end[0] )
			start = end+1;
		else
			start = NULL;
		y += h;	
	}
	if( yout )
		*yout = y;
	if( maxwout )
		*maxwout = maxw;
}

//---------------------------------------------------------------------------

static int CPROC ButtonDraw( PSI_CONTROL pc )
{
	ValidatedControlData( PBUTTON, NORMAL_BUTTON, pb, pc );
	int x;
	if( !pb )
	{
		ValidatedControlData( PBUTTON, CUSTOM_BUTTON, pb2, pc );
      //lprintf( WIDE("Is not a NORMAL_BUTTON") );
		if( pb2 )
			pb = pb2;
		else
		{
			ValidatedControlData( PBUTTON, IMAGE_BUTTON, pb2, pc );
			//lprintf( WIDE("Is not a CUSTOM_BUTTON") );
			if( pb2 )
				pb = pb2;
			else
			{
				//lprintf( WIDE("Is not a IMAGE_BUTTON") );
				return 0;
			}
		}
	}
	//lprintf( WIDE("Button drawing...") );
	if( pb->buttonflags.pressed )
		pc->BorderType |= BORDER_INVERT;
	else
		pc->BorderType &= ~BORDER_INVERT;

	SetDrawBorder( pc );
	if( pc->DrawBorder ) pc->DrawBorder(pc);
	if( !pb->DrawMethod )
	{
		BlatColorAlpha( pc->Surface, 0, 0, pc->surface_rect.width, pc->surface_rect.height, pb->color );
		//ClearImageTo( pc->Surface, pb->color );

		//lprintf( WIDE("drawing an image %p"), pb->pImage );
		if( pb->pImage )
		{
			if( pc->flags.bDisable )
				BlotImageEx( pc->Surface, pb->pImage, 0, 0
							  , TRUE
							  , BLOT_MULTISHADE, Color( 62, 62, 62 )
							  , Color( 67,67,67 )
							  , Color( 60, 60, 60 ) );
			else
				BlotImage( pc->Surface, pb->pImage, 0, 0 );
			if( pc->flags.bFocused )
			{
				_32 width, height;
				GetImageSize( pb->pImage, &width, &height );
				do_line( pc->Surface, 2, height - 2
						 , width - 2, height - 2
						 , basecolor(pc)[SHADE] );
			}
		}
		if( pc->caption.text )
		{
			SFTFont font = GetFrameFont( pc );

			if( pb->buttonflags.pressed )
				x = pc->surface_rect.width/2 + 1;
			else
				x = pc->surface_rect.width/2;
			if( !pc->flags.bDisable )
			{
				_32 y, maxw;
				DrawButtonCaption( pc, pb, x, 0, basecolor(pc)[TEXTCOLOR], &y, &maxw, font );
				if( pc->flags.bFocused )
				{
					do_hline( pc->Surface, y-1, x-maxw/2-1, x+maxw/2+1, basecolor(pc)[SHADE] );
					do_hline( pc->Surface, y, x-maxw/2, x+maxw/2, basecolor(pc)[HIGHLIGHT] );
				}
			}
			else
			{
				DrawButtonCaption( pc, pb, x, 0, basecolor(pc)[SHADOW], NULL, NULL, font );
				DrawButtonCaption( pc, pb, x+1, 1, basecolor(pc)[HIGHLIGHT], NULL, NULL, font );
			}
		}
	}
	else  //( pb->DrawMethod )
	{
		//lprintf( WIDE("Calling application's custom draw routine for a button! ------------------") );
		pb->DrawMethod( pb->DrawData, pc );
	}
	return 1;
}

//---------------------------------------------------------------------------

static void CPROC ButtonCaptionChange( PSI_CONTROL pc )
{
	PBUTTON pb;
	_32 height;
	SFTFont font;
	pb = (PBUTTON)pc;
	font = GetFrameFont( pc );
	GetStringSizeFont( GetText( pc->caption.text ), &pb->centx, &height, font );
	pb->centx/=2;
	pb->topy = ( (signed)pc->surface_rect.height - (signed)height ) / 2;
	if( pb->topy < 0 )
		pb->topy = 0;
}

//---------------------------------------------------------------------------
void InvokeButton( PSI_CONTROL pc )
{
	if( pc->nType == NORMAL_BUTTON ||
       pc->nType == CUSTOM_BUTTON ||
		pc->nType == IMAGE_BUTTON ||
		pc->nType == RADIO_BUTTON 
	  )
	{
		PBUTTON  pb = ControlData( PBUTTON, pc );
		//lprintf( "&*&**&**&*&*&*&*&*&*&*&*& button clicked ");
		if( pb->ClickMethod )
		{
			pb->ClickMethod( pb->ClickData, pc );
		}
		{
			int (CPROC *f)(PSI_CONTROL);
			TEXTCHAR mydef[256];
			CTEXTSTR name;
			PCLASSROOT data = NULL;
			snprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY WIDE("/control/rtti/%d/extra click"), pc->nType );
			for( name = GetFirstRegisteredName( mydef, &data );
				 name;
				  name = GetNextRegisteredName( &data ) )
			{
				f = GetRegisteredProcedureExx((PCLASSROOT)data,(CTEXTSTR)NULL,int,name,(PSI_CONTROL));
				if( f )
					f( pc );
			}
		}
	}
}

static int CPROC ButtonMouse( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	ValidatedControlData( PBUTTON, NORMAL_BUTTON, pb, pc );
	if( !pb )
	{
		ValidatedControlData( PBUTTON, CUSTOM_BUTTON, pb2, pc );
		//lprintf( WIDE("Is not a NORMAL_BUTTON") );
		if( pb2 )
			pb = pb2;
		else
		{
			ValidatedControlData( PBUTTON, IMAGE_BUTTON, pb2, pc );
			//lprintf( WIDE("Is not a CUSTOM_BUTTON") );
			if( pb2 )
				pb = pb2;
			else
			{
				//lprintf( WIDE("Is not a IMAGE_BUTTON") );
				return 0;
			}
		}
	}
	//lprintf( WIDE("mouse on a button...") );
	if( pc->flags.bDisable ) // ignore mouse on these...
		return 0;
	if( b == -1 )
	{
		if( pb->buttonflags.pressed )
		{
			lprintf( WIDE("releaseing press state sorta...") );
			pb->buttonflags.pressed = FALSE;
			SmudgeCommon( pc );
		}
		pb->_b = 0;
		return 1;
	}
	if( x < 0
		 || y < 0
		 || SUS_GT( x,S_32,pc->rect.width,_32)
		 || SUS_GT( y,S_32,pc->rect.height,_32) )
	{
		if( pb->buttonflags.pressed )
		{
			//lprintf( WIDE("Releasing button.") );
			pb->buttonflags.pressed = FALSE;
			SmudgeCommon( pc );
		}
		pb->_b = 0; // pretend no mouse buttons..
		return 0;
	}
	if( b & MK_LBUTTON )
	{
		if( !(pb->_b & MK_LBUTTON ) )
		{
			if( !pb->buttonflags.pressed )
			{
				pb->buttonflags.pressed = TRUE;
				SmudgeCommon( pc );
			}
			if( l.flags.bTouchDisplay )
				InvokeButton( pc );
		}
	}
	else
	{
		if( ( pb->_b != INVALID_INDEX ) && (pb->_b & MK_LBUTTON ) )
		{
			pb->_b = b;
			if( pb->buttonflags.pressed )
			{
				pb->buttonflags.pressed = FALSE;
				SmudgeCommon( pc );
			}
			if( !l.flags.bTouchDisplay )
				InvokeButton( pc );
		}
	}
	pb->_b = b;
	return 1;
}

//---------------------------------------------------------------------------

static int CPROC ButtonKeyProc( PSI_CONTROL pc, _32 key )
{
   //printf( WIDE("Key: %08x\n"), key );
	if( key & 0x80000000 )
	{
		int keymod = KEY_MOD( key );
		if( !keymod &&
			( ( key & 0xFF ) == KEY_SPACE ) ||
			( ( key & 0xFF ) == KEY_ENTER ) )
		{
			InvokeButton( pc );
			return 1;
		}
	}
	return 0;
}

//---------------------------------------------------------------------------

#define LISTBOX 1000

static PSI_CONTROL CONTROL_PROPERTIES( Button )( PSI_CONTROL pc )
{
	PSI_CONTROL page = CreateFrame( WIDE("Button")
									 , 0, 0
									 , PROP_WIDTH, PROP_HEIGHT
									 , BORDER_NONE|BORDER_WITHIN
									 , NULL );
	if( page )
	{
		PCLASSROOT data = NULL;
		PSI_CONTROL pList;
		CTEXTSTR name;
		//PCLASSROOT pcr = GetClassRoot( PSI_ROOT_REGISTRY WIDE("/control/Button/Click") );
		MakeTextControl( page, PROP_PAD, PROP_PAD, 116, 12, TXT_STATIC, WIDE("Click Method"), 0 );
		//MakeTextControl( page, 0, PROP_PAD, PROP_PAD, 116, 12, TXT_STATIC, WIDE("Click Method") );
		pList = MakeListBox( page, PROP_PAD, PROP_PAD + 16, PROP_WIDTH - 2*PROP_PAD, 120, LISTBOX, 0 );
		//pList = MakeListBox( page, 0, PROP_PAD, PROP_PAD + 16, PROP_WIDTH - 2*PROP_PAD, 120, LISTBOX );

		for( name = GetFirstRegisteredName( PSI_ROOT_REGISTRY WIDE("/control/Button/Click"), &data ); name; name = GetNextRegisteredName( &data ) )
		{
			PLISTITEM pli = AddListItem( pList, name );
			SetItemData( pli, (PTRSZVAL)GetRegisteredProcedure( PSI_ROOT_REGISTRY WIDE("/control/Button/Click"), int, name, (PTRSZVAL, PCONTROL) ) );
		}
		// maybe a list box or some junk like that....
	}
   return page;
}


//---------------------------------------------------------------------------

static void OnPropertyEditOkay( WIDE( "Button" ) )( PSI_CONTROL pControl, PSI_CONTROL page )
{
	// read the dialog...
	// destruction will happen shortly after this
	// but this should not care fore that...
	PLISTITEM pli = GetSelectedItem( GetControl( page, LISTBOX ) );
	((PBUTTON)pControl)->ClickMethod = (void (CPROC*)(PTRSZVAL,PSI_CONTROL))GetItemData( pli );

}

CTEXTSTR GetMethodName( POINTER Method, CTEXTSTR type, CTEXTSTR method )
{
	CTEXTSTR name;
	PCLASSROOT data = NULL;
	TEXTCHAR buffer[256];
	if( !Method )
		return NULL;
	snprintf( buffer, sizeof( buffer ), PSI_ROOT_REGISTRY WIDE("/methods/%s/%s"), type, method );
	for( name = GetFirstRegisteredName( buffer, &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		POINTER p;
		//lprintf( WIDE("Checking method name %s %s"), buffer, name );
		p = (POINTER)GetRegisteredProcedureExxx( (PCLASSROOT)NULL, buffer
											  , WIDE("void")
											  , name
											  , WIDE("(PTRSZVAL, PSI_CONTROL)") );
		//if( !p )
		//	lprintf( WIDE("Failed to get procedure ... %s"), name );
		//else
		//	lprintf( WIDE("is %s p(%p) == check(%p)"), name, p, Method );
		if( p == Method )
			break;
	}
	return name;
}

CTEXTSTR GetClickMethodName( void (CPROC*ClickMethod)(PTRSZVAL psv, PCONTROL pc) )
{
	return (CTEXTSTR)GetMethodName( (POINTER)ClickMethod, WIDE("Button"), WIDE("Click") );
}

CTEXTSTR GetDrawMethodName( void (CPROC*DrawMethod)(PTRSZVAL psv, PCONTROL pc) )
{
	return GetMethodName( (POINTER)DrawMethod, WIDE("Button"), WIDE("Draw") );
}

CTEXTSTR GetCheckMethodName( void (CPROC*CheckMethod)(PTRSZVAL psv, PCONTROL pc) )
{
	return GetMethodName( (POINTER)CheckMethod, WIDE("Button"), WIDE("Check") );
}

//---------------------------------------------------------------------------
void CPROC ButtonText( PSI_CONTROL pc, PVARTEXT pvt )
{
	ValidatedControlData( PBUTTON, NORMAL_BUTTON, pb, pc );
	if( !pb )
	{
		ValidatedControlData( PBUTTON, CUSTOM_BUTTON, pb2, pc );
		pb = pb2;
	}
	if( !pb )
	{
		ValidatedControlData( PBUTTON, IMAGE_BUTTON, pb2, pc );
		pb = pb2;
	}
	vtprintf( pvt ,WIDE("#%08x"), pb->color );
	{
		CTEXTSTR name;
		name = pb->ClickMethodName;
		if( name )
			vtprintf( pvt, WIDE(" C\'%s\'"), name );
		name = pb->DrawMethodName;
		if( name )
			vtprintf( pvt, WIDE(" D\'%s\'"), name );
	}

}

//---------------------------------------------------------------------------
void CPROC ButtonLoad( PSI_CONTROL pc, CTEXTSTR line )
{
	//ValidatedControlData( PBUTTON, NORMAL_BUTTON, pb, pc );
	//PBUTTON pb = (PBUTTON)pc;
	DebugBreak();
	//sscanf( line ,WIDE("#%")_32fs WIDE("" ), &pb->color );
}

//---------------------------------------------------------------------------
#undef MakeButton

CONTROL_PROC_DEF_EX( NORMAL_BUTTON, BUTTON, Button, (void) )
{
	ValidatedControlData( PBUTTON, NORMAL_BUTTON, pb, pc );
   Init();
	if( pb )
	{
		// this bit of code needs to go away...
		SetCommonTransparent( pc, TRUE );
		pb->buttonflags.pressed = FALSE;
		pb->color = basecolor(pc)[NORMAL];
		pb->_b = 0;
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------

int CPROC ConfigureCustomDrawnButton( PSI_CONTROL pc )
{
   ValidatedControlData( PBUTTON, CUSTOM_BUTTON, pb, pc );
   //ARG( _32, attr );
	//FP_ARG( void, DrawMethod,(PTRSZVAL psv, PCONTROL pc) );
   //FP_ARG( void, PushMethod,(PTRSZVAL psv, PCONTROL pc) );
	//ARG( PTRSZVAL, Data );
	Init();
#if 0
	if( pb )
	{
		pb->ClickMethod = PushMethod;
		pb->ClickData = Data;
		pb->ClickMethodName = GetClickMethodName( PushMethod );
		if( !pb->ClickMethodName )
		{
			//lprintf( WIDE("!!! Click method is not registered, loading this frame will not result in a link for this method...") );
		}
		pb->DrawMethod = DrawMethod;
		pb->DrawMethodName = GetDrawMethodName( DrawMethod );
		if( !pb->DrawMethodName )
		{
			//lprintf( WIDE("!!! Draw method is not registered, loading this frame will not result in a link for this method...") );
		}
		if( attr & BUTTON_NO_BORDER )
			SetCommonBorder( pc, BORDER_NONE );
	}
#endif
	if( pb )
	{
		SetCommonTransparent( pc, TRUE );
		pc->CaptionChanged = ButtonCaptionChange;
		pb->color = basecolor(pc)[NORMAL];
		pb->buttonflags.pressed = FALSE;
		pb->_b = 0;
		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------

int CPROC ConfigureImageButton( PSI_CONTROL pc )
{
	//ARG( Image, pImage );
	//ARG( _32, attr );
	//FP_ARG( void CPROC,PushMethod,(PTRSZVAL psv, PCONTROL pc) );
	//ARG( PTRSZVAL, Data );
	ValidatedControlData( PBUTTON, IMAGE_BUTTON, pb, pc );
	Init();
	if( pb )
	{
		//if( attr & BUTTON_NO_BORDER )
      //   SetCommonBorder( pc, BORDER_NONE );
		//pb->ClickMethod = PushMethod;
		//pb->ClickData = Data;
		//pb->pImage = pImage;
		pb->color = basecolor(pc)[NORMAL];
		pb->buttonflags.pressed = FALSE;
		pc->CaptionChanged = ButtonCaptionChange;
		return TRUE;
	}
	return FALSE;
}

PSI_PROC( PSI_CONTROL, SetButtonImage )( PSI_CONTROL pc, Image pImage )
{
	ValidatedControlData( PBUTTON, IMAGE_BUTTON, pb, pc );
	if( pb )
	{
		pb->pImage = pImage;
	}
	return pc;
}

//---------------------------------------------------------------------------

void PressButton( PSI_CONTROL pc, int bPressed )
{
	ValidatedControlData( PBUTTON, CUSTOM_BUTTON, pbc, pc );
	ValidatedControlData( PBUTTON, IMAGE_BUTTON, pbi, pc );
	ValidatedControlData( PBUTTON, NORMAL_BUTTON, pbn, pc );
	PBUTTON pb;
	pb = pbc?pbc:pbi?pbi:pbn?pbn:NULL;

	if( pb )
	{
		pb->buttonflags.pressed = bPressed;
		SmudgeCommon( pc );
	}
}

//------------------------------------------------------------------------------

int IsButtonPressed( PSI_CONTROL pc )
{
	ValidatedControlData( PBUTTON, CUSTOM_BUTTON, pb, pc );
	if( pb )
	{
		return pb->buttonflags.pressed;
	}
	return 0;
}

PSI_PROC( void, SetButtonColor )( PSI_CONTROL pc, CDATA color )
{
	ValidatedControlData( PBUTTON, NORMAL_BUTTON, pb, pc );
	if( pb )
	{
		pb->color = color;
		SmudgeCommon( pc );
	}
}
//------------------------------------------------------------------------------
//---------------------------------------------------------------------------
void CPROC RadioButtonText( PSI_CONTROL pc, PVARTEXT pvt )
{
	ValidatedControlData( PCHECK, RADIO_BUTTON, pb, pc );
	if( !pb )
	{
		return;
	}
	//vtprintf( pvt ,"#%08x", pb->color );
	{
		CTEXTSTR name;
		name = pb->ClickMethodName;
		if( name )
			vtprintf( pvt, WIDE(" C\'%s\'"), name );
	}

}

//---------------------------------------------------------------------------

static int CPROC DrawCheckButton( PSI_CONTROL pc )
{
	ValidatedControlData( PCHECK, RADIO_BUTTON, pchk, pc );
	// can be several flavors for check buttons 
	// a> a box [ ]
	// b> a button <> which goes into a pressed state
	// c> the whole control si a button that locks down...
	if( pchk )
	{
		BlatColorAlpha( pc->Surface, 0, 0, pc->surface_rect.width, pc->surface_rect.height, basecolor(pc)[NORMAL] );
		//ClearImageTo( pc->Surface, basecolor(pc)[NORMAL] );
	}
   if( pchk->pCheckWindow )
		DrawThinFrameInvertedImage( pc, pchk->pCheckWindow );

	// this disables also...
	//ClearImageTo( pchk->pCheckSurface, basecolor(pc)[NORMAL]  );

	if( pchk->pCheckSurface )
		if( !pchk->flags.pressed )
		{
			if( pchk->flags.bChecked )
				DrawThinnerFrameInvertedImage( pc, pchk->pCheckSurface );
			else
				DrawThinnerFrameImage( pc, pchk->pCheckSurface );
		}

	if( pc->caption.text )
	{
		int x;
		x = 16;
		if( !pc->flags.bDisable )
		{
			PutMenuString( pc->Surface, x, 0, basecolor(pc)[TEXTCOLOR], 0, GetText( pc->caption.text) );
			if( pc->flags.bFocused )
			{
				_32 end, h;
				end = x + GetStringSize( GetText( pc->caption.text), NULL, &h );
				//h += ;
				do_line( pc->Surface, x, h, end, h, basecolor(pc)[SHADE] );
			}
		}
		else
		{
			PutMenuString( pc->Surface, x+1, 1, basecolor(pc)[HIGHLIGHT], 0, GetText( pc->caption.text) );
			PutMenuString( pc->Surface, x, 0, basecolor(pc)[SHADOW], 0, GetText( pc->caption.text) );
		}
	}
	return 1;
}

//---------------------------------------------------------------------------

static int CPROC CheckKeyProc( PSI_CONTROL pc, _32 key )
{
	PCHECK pb = (PCHECK)pc;
	//printf( WIDE("Key: %08x\n"), key );
	if( key & 0x80000000 )
	{
		if( ( ( key & 0xFF ) == KEY_SPACE ) ||
		    ( ( key & 0xFF ) == KEY_ENTER ) )
		{
			pb->flags.bChecked = !pb->flags.bChecked;
			if( pb->ClickMethod )
				pb->ClickMethod( pb->ClickData, pc );
			SmudgeCommon( pc );
			return 1;
		}
	}
	return 0;
}

//---------------------------------------------------------------------------

static int CPROC MouseCheckButton( PSI_CONTROL pCom, S_32 x, S_32 y, _32 b )
{
	ValidatedControlData( PCHECK, RADIO_BUTTON, pc, pCom );
	if( !pc )
		return 0;
	if( pCom->flags.bDisable ) // ignore mouse on these...
		return 0;
	if( b == -1 )
	{
		pc->flags.pressed = FALSE;
		SmudgeCommon( pCom );
		pc->_b = 0;
		return 1;
	}
	if( x < 0 || y < 0 || SUS_GT(x,S_32, pCom->rect.width,_32) || SUS_GT(y,S_32, pCom->rect.height,_32 ) )
	{
		pc->flags.pressed = FALSE;
		SmudgeCommon( pCom );
		pc->_b = 0; // pretend no mouse buttons..
		return 1;
	}
	if( b && MK_LBUTTON )
	{
		if( !(pc->_b & MK_LBUTTON ) )
		{
			pc->flags.pressed = TRUE;
			SmudgeCommon( pCom );
		}
	}
	else
	{
		if( (pc->_b & MK_LBUTTON ) )
		{
			pc->flags.pressed = FALSE;
			if( pc->groupid && pCom->parent )
			{
				// this should handle recursiveness...
				// but for now we can go just one up, and back down to the 
				// start of the list...
				PSI_CONTROL pcCheck = pCom->parent->child;
				PCHECK pcheck;
				while( ( pcheck = ControlData( PCHECK, pcCheck ) ) )
				{
					if( pCom->nType == RADIO_BUTTON )
					{
						int bChanged = 0;
						if( pcheck->groupid == pc->groupid )
						{
							if( pcheck == pc )
							{
								if( !pcheck->flags.bChecked )
								{
									bChanged = RADIO_CALL_CHECKED;
									pcheck->flags.bChecked = TRUE;
                           SmudgeCommon( pcCheck );
								}
							}
							else
							{
								if( pcheck->flags.bChecked )
								{
									bChanged = RADIO_CALL_UNCHECKED;
									pcheck->flags.bChecked = FALSE;
                           SmudgeCommon( pcCheck );
								}
							}
						}
						if( pc->flags.bCallAll ||
						    ( pc->flags.bCallChecked && bChanged == RADIO_CALL_CHECKED ) ||
						    ( pc->flags.bCallUnchecked && bChanged == RADIO_CALL_UNCHECKED ) )
	      			{
							if( pc->ClickMethod )
								pc->ClickMethod( pc->ClickData, pCom );
						}
					}
					pcCheck = pcCheck->next;
				}
			}
			else
			{
				pc->flags.bChecked = !pc->flags.bChecked;
				if( pc->ClickMethod )
					pc->ClickMethod( pc->ClickData, pCom );
			}
			SmudgeCommon( pCom );
		}
	}
	pc->_b = b;
	return 1;
}

//---------------------------------------------------------------------------

static void CPROC DestroyCheckButton( PSI_CONTROL pc )
{
	ValidatedControlData( PCHECK, RADIO_BUTTON, pchk, pc );
	if( pchk )
	{
		if( pchk->pCheckSurface )
			UnmakeImageFile( pchk->pCheckSurface );
		pchk->pCheckSurface = NULL;
		if( pchk->pCheckWindow )
			UnmakeImageFile( pchk->pCheckWindow );
		pchk->pCheckWindow = NULL;
	}
}

//---------------------------------------------------------------------------

PCONTROL SetButtonGroupID(PCONTROL pControl, int nGroupID )
{
	ValidatedControlData( PCHECK, RADIO_BUTTON, pc, pControl );
	if( pc )
	{
		pc->groupid = nGroupID;
	}
	return pControl;
}
PCONTROL SetCheckButtonHandler( PCONTROL pControl
														, void (CPROC*CheckProc)(PTRSZVAL psv, PCONTROL pc)
														, PTRSZVAL psv )
{
	ValidatedControlData( PCHECK, RADIO_BUTTON, pc, pControl );
	if( pc )
	{
		pc->ClickMethod = CheckProc;
		pc->ClickMethodName = GetCheckMethodName( CheckProc );
		pc->ClickData = psv;
	}
	return pControl;
}


int CPROC ConfigureCheckButton( PSI_CONTROL pControl )
{
	//ARG( _32, attr );
	//ARG( _32, GroupID );
	//FP_ARG(  void CPROC,CheckProc,(PTRSZVAL psv, PSI_CONTROL pc ) );
	//ARG( PTRSZVAL, psv );
	ValidatedControlData( PCHECK, RADIO_BUTTON, pc, pControl );
	Init();
	if( pc )
	{
		pc->pCheckWindow = MakeSubImage( pControl->Surface, 1, 1, 12, 12 );
		pc->pCheckSurface = MakeSubImage( pc->pCheckWindow, 1, 1, 10, 10 );
		//pc->ClickMethod = CheckProc;
		//pc->ClickMethodName = GetCheckMethodName( CheckProc );
		//pc->ClickData = psv;
		//pc->groupid = GroupID;
		/*
	   if( !( attr & RADIO_CALL_CHANGED ) )
			pc->flags.bCallAll = 1;
		else
		{
			if( attr & RADIO_CALL_CHECKED )
				pc->flags.bCallChecked = 1;
			if( attr & RADIO_CALL_UNCHECKED )
				pc->flags.bCallUnchecked = 1;
		}
		*/
		return 1;
	}
	return 0;
}

//---------------------------------------------------------------------------

int GetCheckState( PSI_CONTROL pc )
{
	ValidatedControlData( PCHECK, RADIO_BUTTON, pchk, pc );
	if( pchk )
	{
		return pchk->flags.bChecked;
	}
	return 0;
}

//---------------------------------------------------------------------------

void SetCheckState( PSI_CONTROL pc, int nState )
{
	ValidatedControlData( PCHECK, RADIO_BUTTON, pchk, pc );
	if( pchk )
	{
		pchk->flags.bChecked = nState;
		SmudgeCommon( pc );
	}
}

//---------------------------------------------------------------------------
void GetButtonPushMethod( PCONTROL pc, ButtonPushMethod *method, PTRSZVAL *psv )
{
	ValidatedControlData( PBUTTON, NORMAL_BUTTON, pb, pc );
	if( !pb )
	{
		ValidatedControlData( PBUTTON, CUSTOM_BUTTON, pb2, pc );
		//lprintf( WIDE("Is not a NORMAL_BUTTON") );
		if( pb2 )
			pb = pb2;
		else
		{
			ValidatedControlData( PBUTTON, IMAGE_BUTTON, pb2, pc );
			//lprintf( WIDE("Is not a CUSTOM_BUTTON") );
			if( pb2 )
				pb = pb2;
			else
			{
				//lprintf( WIDE("Is not a IMAGE_BUTTON") );
				return;
			}
		}
	}
	if( pb )
	{
		if( method )
			(*method) = pb->ClickMethod;
		if( psv )
			(*psv) = pb->ClickData;
	}

}

PCONTROL SetButtonAttributes( PCONTROL pCom, int attr )
{
	// BUTTON_ flags...
	if( attr & BUTTON_NO_BORDER )
		SetCommonBorder( pCom, BORDER_NONE );
	//else
	//   SetCommonBorder( pc, BORDER_DEFAULT_FOR_BUTTON ); // LOL - someday
	{
		ValidatedControlData( PCHECK, RADIO_BUTTON, pc, pCom );
		if( pc )
		{
			if( attr & RADIO_CALL_CHANGED )
				pc->flags.bCallAll = 1;
			else
			{
				if( attr & RADIO_CALL_CHECKED )
					pc->flags.bCallChecked = 1;
				if( attr & RADIO_CALL_UNCHECKED )
					pc->flags.bCallUnchecked = 1;
			}
		}

	}
	return pCom;
}

PSI_CONTROL SetButtonDrawMethod( PSI_CONTROL pc, ButtonPushMethod method, PTRSZVAL psv )
{
	ValidatedControlData( PBUTTON, NORMAL_BUTTON, pb, pc );
	if( !pb )
	{
		ValidatedControlData( PBUTTON, CUSTOM_BUTTON, pb2, pc );
		if( pb2 )
			pb = pb2;
	}
	if( pb )
	{
		pb->DrawMethod = method;
		pb->DrawData = psv;
	}
	return pc;
}

PSI_CONTROL SetButtonPushMethod( PSI_CONTROL pc, ButtonPushMethod method, PTRSZVAL psv )
{
	ValidatedControlData( PBUTTON, NORMAL_BUTTON, pb, pc );
	if( !pb )
	{
		ValidatedControlData( PBUTTON, CUSTOM_BUTTON, pb2, pc );
		//lprintf( WIDE("Is not a NORMAL_BUTTON") );
		if( pb2 )
			pb = pb2;
		else
		{
			ValidatedControlData( PBUTTON, IMAGE_BUTTON, pb2, pc );
			//lprintf( WIDE("Is not a CUSTOM_BUTTON") );
			if( pb2 )
				pb = pb2;
			else
			{
				ValidatedControlData( PCHECK, RADIO_BUTTON, pb2, pc );
				//lprintf( WIDE("Is not a CUSTOM_BUTTON") );
				if( pb2 )
				{
					pb2->ClickMethod = method;
					pb2->ClickData = psv;
					return pc;
				}
				else
				{
					//lprintf( WIDE("Is not a IMAGE_BUTTON") );
					return pc;
				}
			}
		}
	}
	if( pb )
	{
		pb->ClickMethod = method;
		pb->ClickData = psv;
	}
	return pc;
}

//---------------------------------------------------------------------------

CONTROL_REGISTRATION
normal_button = { NORMAL_BUTTON_NAME
					 , { {73, 21}, sizeof( BUTTON ), BORDER_THIN|BORDER_NOCAPTION }
					 , InitButton
					 , NULL
					 , ButtonDraw
					 , ButtonMouse
					 , ButtonKeyProc
					 , NULL // destroy ( I have no private dynamic data == NULL )
					 ,NULL //, GetButtonPropertyPage
					 ,NULL //, ApplyButtonPropertyPage
                , ButtonText // save...
					 , NULL // ButtonLoadProc
},
image_button = { IMAGE_BUTTON_NAME
					, { {73, 21}, sizeof( BUTTON ), BORDER_THIN|BORDER_NOCAPTION }
					, ConfigureImageButton// init
               , NULL
					, ButtonDraw
					, ButtonMouse
					, ButtonKeyProc
					, NULL
					, NULL// getpage
					, NULL// apply
					, NULL // save
					, NULL // load
},
custom_button = { CUSTOM_BUTTON_NAME
					 , { {73, 21}, sizeof( BUTTON ), BORDER_THIN|BORDER_NOCAPTION }
					 , ConfigureCustomDrawnButton
					 , NULL
					 , ButtonDraw
					 , ButtonMouse
					 , ButtonKeyProc
					 , NULL
					 , NULL
					 , NULL
                , ButtonText // save...

},
radio_button = { RADIO_BUTTON_NAME
					, { {73, 21}, sizeof( CHECK ) + 100, BORDER_NONE|BORDER_NOCAPTION}
					, ConfigureCheckButton// init
               , NULL
					, DrawCheckButton
					, MouseCheckButton
					, CheckKeyProc
					, DestroyCheckButton
					, NULL
					, NULL
               , RadioButtonText
};


static int OnCommonFocus( NORMAL_BUTTON_NAME )( PSI_CONTROL pc, LOGICAL bFocus )
{
	SmudgeCommon( pc );
	return 1;
}

PRIORITY_PRELOAD( register_buttons, PSI_PRELOAD_PRIORITY ) {
	l.flags.bTouchDisplay = 1;//IsTouchDisplay();
	DoRegisterControl( &normal_button );
	DoRegisterControl( &image_button );
	DoRegisterControl( &custom_button );
	DoRegisterControl( &radio_button );
	//DumpRegisteredNames();
}

PSI_BUTTON_NAMESPACE_END

//---------------------------------------------------------------------------
// $Log: ctlbutton.c,v $
// Revision 1.68  2005/07/05 17:50:55  d3x0r
// fixes to update local region - esp as related to popup menus
//
// Revision 1.67  2005/06/19 08:08:33  d3x0r
// Fix borders on buttons (somehow becaem border THINNER).  Also fix control update portion... was updating the surface x,y with the control width, height, overflowing...
//
// Revision 1.66  2005/05/25 16:50:18  d3x0r
// Synch with working repository.
//
// Revision 1.79  2005/03/22 12:41:58  panther
// Wow this transparency thing is going to rock! :) It was much closer than I had originally thought.  Need a new class of controls though to support click-masks.... oh yeah and buttons which have roundable scaleable edged based off of a dot/circle
//
// Revision 1.78  2005/03/21 20:41:34  panther
// Protect against super large fonts, remove edit frame from palette, and clean up some warnings.
//
// Revision 1.77  2005/03/07 00:47:01  panther
// Fix image buttons.
//
// Revision 1.22  2003/03/25 08:45:56  panther
// Added CVS logging tag
//
