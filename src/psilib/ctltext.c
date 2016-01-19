
#include "controlstruc.h"

#include "controls.h"
#include <psi.h>

PSI_TEXT_NAMESPACE

typedef struct text_tag {
	_32 attr; // centering field...
	struct {
		BIT_FIELD bShadow : 1;
	} flags;
	CDATA foreground, background;
	int offset; // text offset from text anchor position.
} TEXTCONTROL, *PTEXTCONTROL;

//------------------------------------------------------------------------------

PSI_CONTROL SetTextControlAttributes( PSI_CONTROL pc, int flags )
{
	ValidatedControlData( PTEXTCONTROL, STATIC_TEXT, ptc, pc );
	if( ptc )
	{
		ptc->attr = flags;
	}
	return pc;
}

void SetControlAlignment( PSI_CONTROL pc, int align )
{
	ValidatedControlData( PTEXTCONTROL, STATIC_TEXT, ptc, pc );
	//if( align & TEXT_INVERT )
	//	pc->flags.bInvert = 1;
	//else
	//	pc->flags.bInvert = 0;
	if( align & TEXT_VERTICAL )
		pc->flags.bVertical = 1;
	else
		pc->flags.bVertical = 0;
	if( align & TEXT_RIGHT )
		pc->flags.bAlign = 3;
	else if( align & TEXT_CENTER )
		pc->flags.bAlign = 2;
	else
		pc->flags.bAlign = 0;
	if( align & TEXT_SHADOW_TEXT )
		ptc->flags.bShadow = 1;
	SmudgeCommon( pc );
}

static int CPROC OnDrawCommon( STATIC_TEXT_NAME )( PSI_CONTROL pc )
{
	ValidatedControlData( PTEXTCONTROL, STATIC_TEXT, ptc, pc );
	//lprintf( WIDE("drawing a text control...") );
	if( ptc )
	{
		Image surface = GetControlSurface( pc );
		SFTFont font;
		_32 height, width;
		if( ptc->background )
			ClearImageTo( surface, ptc->background );
		font = GetFrameFont( pc );
		GetStringSizeFont( GetText( pc->caption.text )
										 , &width, &height, font );
		if( pc->caption.text ) switch( pc->flags.bAlign )
		{
		case 3: // right align - default to left align for now.
			if( pc->flags.bVertical )
				PutStringVerticalFont( surface
											, ( (int)surface->width - (int)width ) / 2, ( (int)surface->height - (int)height )
											, ptc->foreground, 0
											, GetText( pc->caption.text ), font );
			else
				PutStringFont( surface
								 , (int)ptc->offset + (int)surface->width - (int)width, ( (int)surface->height - (int)height ) / 2
								 , ptc->foreground, 0
								 , GetText( pc->caption.text ), font );
			break;
		default:
		case 0: // default
		case 1: // left
			if( pc->flags.bVertical )
				PutStringVerticalFont( surface
											, ( (int)surface->width - (int)width ) / 2 , 0
											, ptc->foreground, 0
											, GetText( pc->caption.text ), font );
			else
				PutStringFont( surface
								 , ptc->offset, ( (int)surface->height - (int)height ) / 2
								 , ptc->foreground
								 , 0
								 , GetText( pc->caption.text ), font );
			break;
		case 2: // center
			{
				width = GetStringSizeFont( GetText( pc->caption.text )
												 , NULL, &height, font );
				if( pc->flags.bVertical )
					PutStringVerticalFont( surface
												, ( (int)surface->width - (int)height ) / 2
												, ptc->offset + ( (int)surface->height - (int)width ) / 2
												, ptc->foreground
												, 0
												, GetText( pc->caption.text ), font );
				else
				{
					//lprintf( "render string length %d at %d", width, ptc->offset + ( ( (int)surface->width - width ) / 2 ) );
					if( ptc->flags.bShadow )
					{
                  // okay we don't know how to get the shadow color have to extedn that
						PutStringFont( surface
										 , ptc->offset + ( ( (int)surface->width - (int)width ) / 2 )
										 , ( (int)surface->height - (int)height ) / 2
										 , ptc->foreground
										 , 0
										 , GetText( pc->caption.text ), font );
					}
					else
						PutStringFont( surface
										 , ptc->offset + ( ( (int)surface->width - (int)width ) / 2 )
										 , ( (int)surface->height - (int)height ) / 2
										 , ptc->foreground
										 , 0
										 , GetText( pc->caption.text ), font );
				}
			}
			break;
		}
	}
	return TRUE;
}

//---------------------------------------------------------------------------
#undef MakeTextControl
CAPTIONED_CONTROL_PROC_DEF( STATIC_TEXT, TEXTCONTROL, TextControl, (int attr) )
{
	ValidatedControlData( PTEXTCONTROL, STATIC_TEXT, ptc, pc );
	if( ptc )
	{
		SetNoFocus( pc );
		ptc->foreground = basecolor(pc)[TEXTCOLOR];
		ptc->background = basecolor(pc)[NORMAL];
		SetCommonTransparent( pc, TRUE );
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------

PSI_PROC( void, SetTextControlColors )( PSI_CONTROL pc, CDATA fore, CDATA back )
{
	ValidatedControlData( PTEXTCONTROL, STATIC_TEXT, ptc, pc );
	if( (!ptc) )
		return;	
	if( fore )
		ptc->foreground = fore;
	else
		ptc->foreground = basecolor(pc)[TEXTCOLOR];

	// if we don't allow the application to set the background to OFF
	// then it can't just be floating transparency text....
	//if( back )
		ptc->background = back;
	//else
	//	ptc->background = basecolor(pc)[NORMAL];
	SmudgeCommon( pc );
}

LOGICAL GetControlTextOffsetMinMax( PSI_CONTROL pc, int *min_offset, int *max_offset )
{
	ValidatedControlData( PTEXTCONTROL, STATIC_TEXT, ptc, pc );
	if( ptc )
	{
		int minofs;
		int maxofs;
		_32 height, width;
		S_32 _height, _width;
		Image surface = GetControlSurface( pc );
		SFTFont font = GetFrameFont( pc );
		width = GetStringSizeFont( GetText( pc->caption.text )
										 , NULL, &height, font );
		_width = (int)width;
		_height = (int)height;
		minofs = (-_width+1);
		switch(  pc->flags.bAlign )
		{
		case 3: // right align - default to left align for now.
			if( pc->flags.bVertical )
			{
				minofs =((surface->height -_width) -_width ) + 1;
				maxofs = (surface->height) - 1;
			}
			else
			{
				minofs =((surface->width -_width) -_width ) + 1;
				maxofs = (surface->width) - 1;
			}
			break;
		default:
		case 0: // default
		case 1: // left
			if( pc->flags.bVertical )
			{
				minofs = ( -_width ) + 1;
				maxofs = surface->height - 1;
			}
			else
			{
				minofs = ( -_width ) + 1;
				maxofs = surface->width - 1;
			}
			break;
		case 2: // center
			if( pc->flags.bVertical )
			{
				// okay center is (h/2 - w/2) for first pixel
				// min is this posisition minus the width minus this position
				minofs = ( -( _width  + ( ( surface->height - _width ) / 2 )) )+ 1;
				maxofs = ( surface->height-( (int)surface->height - _width )/2  )- 1;
			}
			else
			{
				minofs = ( -( _width  + ( ( surface->width - _width ) / 2 )) )+ 1;
				maxofs = ( surface->width-( (int)surface->width - _width )/2  )- 1;
			}
		}
		if( min_offset )
			(*min_offset) = minofs;
		if( max_offset )
			(*max_offset) = maxofs;
		//lprintf( "result was %d %d", minofs, maxofs );
		return TRUE;
	}
	return FALSE;
}


LOGICAL SetControlTextOffset( PSI_CONTROL pc, int offset )
{
	ValidatedControlData( PTEXTCONTROL, STATIC_TEXT, ptc, pc );
	if( ptc )
	{
		LOGICAL result = TRUE;
		ptc->offset = offset;
		{
			_32 height, width;
			S_32 _height, _width;
			Image surface = GetControlSurface( pc );
			SFTFont font = GetFrameFont( pc );
			width = GetStringSizeFont( GetText( pc->caption.text )
											 , NULL, &height, font );
			_width = (int)width;
			_height = (int)height;
			switch( pc->flags.bAlign )
			{
			case 3: // right align - default to left align for now.
				if( pc->flags.bVertical )
				{
					if( ( offset + ( (int)surface->height - _width ) > surface->height )
						|| ( offset + ( (int)surface->height - _width ) < 0 ) )
					{
						//lprintf( "false at %d %d %d %d",ptc->offset, _width, surface->width, ( ptc->offset + _width + ( (int)surface->width - _width )/2 ) );
						result = FALSE;
					}
				}
				else
				{
						if( ( ( offset + (int)surface->width - _width ) > surface->width )
							|| ( ( offset + (int)surface->width - _width ) < 0 ) )
						{
							//lprintf( "false at %d %d %d %d",ptc->offset, _width, surface->width, ( ptc->offset + _width + ( (int)surface->width - _width )/2 ) );
							result = FALSE;
						}
				}
				break;
			default:
			case 0: // default
			case 1: // left
				if( pc->flags.bVertical )
				{
					if( ( ( offset + ( surface->height - _width ) ) > surface->height )
						|| SUS_GT( ( offset + ( surface->height - _width ) ), S_32, width, _32 ) )
						result = FALSE;
				}
				else
				{
					if( ( offset > surface->width )
						|| ( ( offset + _width ) < 0 ) )
						result = FALSE;
				}
				break;
			case 2: // center
				if( pc->flags.bVertical )
				{
					if( ( ( ( ptc->offset + ( surface->height - _width )/2 ) ) > surface->height )
						|| ( ( ptc->offset + _width + ( ( surface->height - _width )/2 ) ) < 0 ) )
					{
						result = FALSE;
					}
				}
				else
				{
					if( ( ( ( ptc->offset + ( (int)surface->width - _width )/2 ) ) > surface->width )
						|| ( ( ptc->offset + _width + ( ( (int)surface->width - _width )/2 ) ) < 0 ) )
					{
                  //lprintf( "false at %d %d %d %d",ptc->offset, _width, surface->width, ( ptc->offset + _width + ( (int)surface->width - _width )/2 ) );
						result = FALSE;
					}
				}
			}
		}
		SmudgeCommon( pc );
		return result;
	}
	return FALSE;
}


#include <psi.h>
CONTROL_REGISTRATION
text_control = { STATIC_TEXT_NAME
               , { {73, 21}, sizeof( TEXTCONTROL ), BORDER_NONE}
               , InitTextControl// init
               , NULL
               , NULL //_DrawTextControl
               , NULL
               , NULL
};

PRIORITY_PRELOAD( register_text, PSI_PRELOAD_PRIORITY ) {
	DoRegisterControl( &text_control );
}

PSI_TEXT_NAMESPACE_END

//---------------------------------------------------------------------------

