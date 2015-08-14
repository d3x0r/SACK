#ifndef CONTROL_SOURCE
#define CONTROL_SOURCE
#include <stdhdrs.h>
#include <stdarg.h>
#include <stdio.h>
#include <procreg.h>
#include <fractions.h>
#include "global.h"
#include <controls.h>



SACK_NAMESPACE

_PSI_NAMESPACE
// define this to prevent multiple definition to application 
// viewpoint...
//---------------------------------------------------------------------------

// default_width, default_height,
#define CONTROL_PROC_DEF( controltype, type, name, _args )                  \
	int CPROC Init##name( PSI_CONTROL pControl );  \
	int CPROC Config##name( PSI_CONTROL pc ) { return Init##name(pc); } \
	int CPROC Init##name( PSI_CONTROL pc )

#define CAPTIONED_CONTROL_PROC_DEF( controltype, type, name, _args )                  \
	int CPROC Init##name( PSI_CONTROL pControl );  \
	int CPROC Config##name( PSI_CONTROL pc ) { return Init##name(pc); } \
	int CPROC Init##name( PSI_CONTROL pc )


#define CONTROL_PROC_DEF_EX( controltype, type, name, _args) \
	int CPROC Init##name( PSI_CONTROL );  \
	int CPROC Config##name( PSI_CONTROL pc ) { return Init##name(pc); } \
	int CPROC Init##name( PSI_CONTROL pc )

#define ARG( type, name ) PARAM( args, type, name )
#define FP_ARG( type, name, funcargs ) FP_PARAM( args, type, name, funcargs )


#ifdef GCC
#define CONTROL_INIT(name) CONTROL_PROPERTIES( name )(PSI_CONTROL pControl)      \
{ return NULL; } CONTROL_PROPERTIES_APPLY( name )(PSI_CONTROL pc,PSI_CONTROL page) { ; }    \
	int CPROC Init##name ( PTRSZVAL psv, PSI_CONTROL pControl, _32 ID )

#define CONTROL_INIT_EX(name)  \
	int CPROC Init##name ( PTRSZVAL psv, PSI_CONTROL pControl, _32 ID )
//#error Need to figure out how to register control Inits

#else
#define CONTROL_INIT(name)  CONTROL_PROPERTIES( name )(PSI_CONTROL pc)      \
{ return NULL; } CONTROL_PROPERTIES_APPLY( name )(PSI_CONTROL pc,PSI_CONTROL page) { ; }   \
	int CPROC Init##name ( PSI_CONTROL pc, va_list args )

#define CONTROL_INIT_EX(name)  \
	int CPROC Init##name ( PSI_CONTROL pc, va_list args )
//#error Need to figure out how to register control Inits
#endif



// size of the property dialog page...
#define PROP_WIDTH 540
#define PROP_HEIGHT 240
#define PROP_PAD 5


/* \Internal event callback definition. Request an additional
	page to add to the control property edit dialog.           */
typedef PSI_CONTROL (CPROC*GetControlPropSheet)( PSI_CONTROL );
#define CONTROL_PROPERTIES( name )  OnPropertyEdit( TOSTR(name) )

/* \Internal defintion of the callback to invoke when a property
	sheet is requested to be applied when editing the control.    */
typedef void (CPROC*ApplyControlPropSheet)( PSI_CONTROL, PSI_CONTROL );
#define CONTROL_PROPERTIES_APPLY( name )  OnPropertyEditOkay( TOSTR(name) )
/* Tells a control that the edit process is done with the
	property sheet requested.
	
	\Internal event callback definition.                   */
typedef void (CPROC*DoneControlPropSheet)( PSI_CONTROL );
#define CONTROL_PROPERTIES_DONE( name )  OnPropertyEditDone( TOSTR(name) )

//typedef struct subclass_control_tag {
//	PCONTROL_REGISTRATION methods;
//};

//#define ControlType(pc) ((pc)->nType)

enum HotspotLocations {
	SPOT_NONE // 0 = no spot locked...
	 , SPOT_TOP_LEFT
	 , SPOT_TOP
	 , SPOT_TOP_RIGHT
	 , SPOT_LEFT
	 , SPOT_CENTER
	 , SPOT_RIGHT
	 , SPOT_BOTTOM_LEFT
	 , SPOT_BOTTOM
	 , SPOT_BOTTOM_RIGHT
};

/* \Internal event callback definition. Draw border, this
	usually pointing to an internal function, but may be used for
	a control to draw a custom border.                            */
typedef void (CPROC*_DrawBorder)        ( struct common_control_frame * );
/* \Event callback definition. Draw caption, set with OnCaptionDraw().
    Use SetCaptionHeight() when creating the frame to set the height of the
	custom caption.
*/
typedef void (CPROC*_DrawCaption)        ( struct common_control_frame *, Image );
/* \Internal event callback definition. This is called when the
	control needs to draw itself. This happens when SmudgeCommon
	is called on the control or on a parent of the control.      */
typedef int (CPROC*__DrawThySelf)       ( struct common_control_frame * );
/* \Internal event callback definition. This is called when the
	control needs to draw itself. This happens when SmudgeCommon
	is called on the control or on a parent of the control.      */
typedef void (CPROC*__DrawDecorations)       ( struct common_control_frame * );
/* \Internal event callback definition. A mouse event is
	happening over the control.                           */
typedef int (CPROC*__MouseMethod)       ( struct common_control_frame *, S_32 x, S_32 y, _32 b );
/* \Internal event callback definition. A key has been pressed. */
typedef int (CPROC*__KeyProc)           ( struct common_control_frame *, _32 );
/* \Internal event callback definition. The caption of a control
	is changing (Edit control uses this).                         */
typedef void (CPROC*_CaptionChanged)    ( struct common_control_frame * );
/* \Internal event callback definition. Destruction of the
	control is in progress. Allow control to free internal
	resources.                                              */
typedef void (CPROC*_Destroy)           ( struct common_control_frame * );
/* \Internal event callback definition.
	
	A control has been added to this control. */
typedef void (CPROC*_AddedControl)      ( struct common_control_frame *, struct common_control_frame *pcAdding );
/* \Internal event callback definition. The focus of a control
	is changing.                                                */
typedef void (CPROC*_ChangeFocus)       ( struct common_control_frame *, LOGICAL bFocused );
/* \Internal event callback definition. Called when a control is
		being resized. Width or height changing.                      */
typedef void (CPROC*_Resize)            ( struct common_control_frame *, LOGICAL bSizing );
typedef void (CPROC*_Move)            ( struct common_control_frame *, LOGICAL bSizing );
typedef void (CPROC*_Rescale)            ( struct common_control_frame * );
/* \Internal event callback definition. Called when the
	control's position (x,y) is changing.                */
typedef void (CPROC*_PosChanging)       ( struct common_control_frame *, LOGICAL bMoving );	
/* \Internal event callback definition. Triggered when edit on a
	frame is started.                                             */
typedef void (CPROC*_BeginEdit)         ( struct common_control_frame * );
/* \Internal event callback definition. Ending control editing. */
typedef void (CPROC*_EndEdit)           ( struct common_control_frame * );
/* \Internal event callback definition. A file has been dropped
	on the control.                                              */
typedef LOGICAL (CPROC*_AcceptDroppedFiles)( struct common_control_frame *, CTEXTSTR filename, S_32 x, S_32 y );


#define DeclMethod( name ) int n##name; _##name *name
#define DeclSingleMethod( name ) _##name name
// right now - all these methods evolved from a void
// function, therefore, this needs to invoke all key/draw methods not just
// until 'processed'
#define InvokeDrawMethod(pc,name,args)  if( (pc) && (pc)->name ) { int n; for( n = 0; (pc) && n < (pc)->n##name; n++ ) if( (pc)->name[n] ) /*if(*/(pc)->draw_result |= (pc)->name[n]args /*)*/ /*break*/; }
#define InvokeMethod(pc,name,args)  if( (pc) && (pc)->name ) { int n; for( n = 0; (pc) && n < (pc)->n##name; n++ ) if( (pc)->name[n] ) /*if(*/(pc)->name[n]args /*)*/ /*break*/; }
#define InvokeResultingMethod(result,pc,name,args)  if( (pc) && (pc)->name ) { int n; for( n = 0; (pc) && n < (pc)->n##name; n++ ) if( (pc)->name[n] ) if( (result)=(pc)->name[n]args ) break; }
#define InvokeSingleMethod(pc,name,args)  if( (pc)->name ) { (pc)->name args; }

void InvokeControlHidden( PSI_CONTROL pc );
void InvokeControlRevealed( PSI_CONTROL pc );


struct edit_state_tag {

//DOM-IGNORE-BEGIN
	PSI_CONTROL pCurrent;
	// so we can restore keystrokes to the control
	// and/or relay unused keys to the control...
	// perhaps should override draw this way
	// some controls may take a long time to draw while 
	// sizing... although we probably do want to see
	// their behavior at the new size....
	// but THIS definatly so we can process arrow keys...
	//void (CPROC*PriorKeyProc)( PTRSZVAL psv, _32 );
	//PTRSZVAL psvPriorKey;
	DeclMethod( _KeyProc );
	//void (CPROC*_PriorKeyProc)( PSI_CONTROL pc, _32 );

	IMAGE_POINT hotspot[9];
	IMAGE_RECTANGLE bound;
	IMAGE_POINT bias; // pCurrent upper left corner kinda on the master frame
	S_32 _x, _y; // marked x, y when the hotspot was grabbed...

	// should also do a cumulative change delta off
	// this to lock the mouse in position...
	S_32 delxaccum, delyaccum;

	struct {
		BIT_FIELD bActive : 1; // edit state is active.
		BIT_FIELD fLocked : 4; // which spot it's locked on...
		BIT_FIELD bDragging : 1;
		BIT_FIELD bSizing : 1; // any sizing flag set
		BIT_FIELD bSizing_left  : 1;
		BIT_FIELD bSizing_right : 1;
		BIT_FIELD bSizing_top   : 1;
		BIT_FIELD bSizing_bottom: 1;
		BIT_FIELD bFrameWasResizable : 1;
		BIT_FIELD bHotSpotsActive : 1;
	} flags;
	_32 BorderType;

//DOM-IGNORE-END
};
typedef struct edit_state_tag EDIT_STATE;
typedef struct edit_state_tag *PEDIT_STATE;

struct physical_device_caption_button
{
	Image normal;
	Image pressed;
	Image highlight;
	void (CPROC*pressed_event)( PSI_CONTROL pc );
	LOGICAL is_pressed;
	int extra_pad;
	_32 offset;
	struct {
		BIT_FIELD hidden : 1;
		BIT_FIELD rollover : 1;
	} flags;
	PSI_CONTROL pc;
};
typedef struct physical_device_caption_button CAPTION_BUTTON;
typedef struct physical_device_caption_button *PCAPTION_BUTTON;


typedef struct frame_border {
	CDATA *defaultcolors;
	S_32 BorderWidth;
	S_32 BorderHeight;
	struct psi_global_border_info {
		BIT_FIELD bAnchorTop : 2; // 0 = none, 1=left, 2=center, 3=right
		BIT_FIELD bAnchorBottom : 2; // 0 = none, 1=left, 2=center, 3=right
		BIT_FIELD bAnchorLeft : 2; // 0 = none, 1=top, 2=center, 3=bottom
		BIT_FIELD bAnchorRight : 2; // 0 = none, 1=top, 2=center, 3=bottom
	} Border;
	Image BorderImage;
	Image BorderSegment[9]; // really 8, but symetry is kept

} FrameBorder;


struct physical_device_interface
{
//DOM-IGNORE-BEGIN
	PRENDERER pActImg; // any control can have a physical renderer...
	struct common_control_frame * common; // need this to easily back track...
	struct device_flags {
		BIT_FIELD bDragging : 1; // frame is being moved
		BIT_FIELD bSizing       : 1; // flags for when frame is sizable
		BIT_FIELD bSizing_left  : 1;
		BIT_FIELD bSizing_right : 1;
		BIT_FIELD bSizing_top   : 1;
		BIT_FIELD bSizing_bottom: 1;
		BIT_FIELD bCurrentOwns : 1; // pCurrent is also owner of the mouse (button was clicked, and never released)
		BIT_FIELD bNoUpdate : 1; // don't call update function...
		BIT_FIELD bCaptured : 1; // frame owns mouse, control behaving as frame wants all mouse events.
		BIT_FIELD bApplicationOwned : 1; // current owns was set by application, do not auto disown.
		BIT_FIELD sent_redraw : 1; // stops sending multiple redraw events....
	}flags;
	EDIT_STATE EditState;
	//PRENDERER pActImg;
	//PTRSZVAL psvUser; // user data...
	int drag_x, drag_y; // position drag was started; for absolute motion
	int _x, _y;
	_32 _b; // last button state...
	// these two buttons override controls which have the ID BTN_OKAY, BTN_CANCEL
	int nIDDefaultOK;
	int nIDDefaultCancel;
	// when has mouse is set, this is set
	// as a quick computation of the coordinate
	// bias.
	struct {
		struct {
			BIT_FIELD bias_is_surface : 1;
		} flags;
		S_32 x, y;
	} CurrentBias;
	//Image original_surface;
	struct common_control_frame * pCurrent; // Current control which has the mouse within it...(or owns mouse)
	struct common_control_frame * pFocus;   // keyboard goes here...
	// this is now added as a draw callback method
	//void (CPROC*OwnerDraw)(struct common_control_frame * pc);
	// this is now added as a mouse callback method
	//void (CPROC*OwnerMouse)(struct common_control_frame * pc, S_32 x, S_32 y, _32 b);
	// this is unused yet...
	//int (CPROC*InitControl)(PTRSZVAL, struct common_control_frame *, _32);// match ControlInitProc(controls.h)
	PTRSZVAL psvInit;
	PLIST pending_dirty_controls; // optimized search list for (allow_threaded_draw == FALSE)
//DOM-IGNORE-END
};
typedef struct physical_device_interface PHYSICAL_DEVICE;
typedef struct physical_device_interface*PPHYSICAL_DEVICE;




typedef struct common_button_data {
//DOM-IGNORE-BEGIN
	PTHREAD thread;
	int *okay_value;
	int *done_value;
	struct button_flags {
		_32 bWaitOnEdit : 1;
	} flags;
//DOM-IGNORE-END
} COMMON_BUTTON_DATA, *PCOMMON_BUTTON_DATA;


//DOM-IGNORE-BEGIN

typedef struct common_control_frame
{
	// this is the one place allowed for
	// an application to store data.
	// once upon a time, I followed windows and
	// allowed set/get network long, which allowed
	// multiple values to be kept...
	// but that doesn't help anything, it just allows for
	// bad coding, and useless calls to fetch values
	// from the object...
	POINTER pUser;
	// the user may also set a DWORD value associated with a control.
	PTRSZVAL psvUser;

	// this is the numeric type ID of the control.  Although, now
	// controls are mostly tracked with their name.
	int nType; 
	/* Name of the type this control is. Even if a control is
		created by numeric Type ID, it still gets its name from the
		procedure registry.                                         */
	CTEXTSTR pTypeName;
	// unique control ID ....
	int nID;
	// this is the text ID registered...
	CTEXTSTR pIDName; 
	//----------------
	// the data above this point may be known by
	// external sources...

	/* just the data portion of the control */
	Image Surface;
	Image OriginalSurface; // just the data portion of the control

	/* flags that affect a control's behavior or state.
		                                                 */
	/* <combine sack::psi::common_control_frame::flags@1>
		
		\ \                                                */
	struct {
		/* Control is currently keyboard focused. */
		BIT_FIELD bFocused : 1;
		// destroyed - and at next opportunity will be...
		BIT_FIELD bDestroy : 1; 
		// set when a size op begins to void draw done during size
		BIT_FIELD bSizing : 1; 
		// used to make Frame more 'Pop' Up...
		BIT_FIELD bInitial : 1; 
		// set to disable updates
		BIT_FIELD bNoUpdate : 1;
		// this control was explicitly set hidden.. don't unhide.
		BIT_FIELD bHiddenParent : 1; 
		// can't see it, can't touch it.
		BIT_FIELD bHidden : 1; 
		// scale currently applies.
		BIT_FIELD bScaled : 1; 
		/* control gets no keyboard focus. */
		BIT_FIELD bNoFocus:1;
		// greyed out state?
		BIT_FIELD bDisable : 1; 
		 // 0 = default alignment 1 = left, 2 = center 3 = right
		BIT_FIELD bAlign:2;
		// draw veritcal instead of horizontal
		BIT_FIELD bVertical:1;
		 // draw opposite/upside down from normal
		BIT_FIELD bInvert:1;
		// needs DrawThySelf called...
		BIT_FIELD bDirty : 1;
		// DrawThySelf has been called...
		BIT_FIELD bCleaning : 1;
		// only need to update the control's frame... (focus change)
		BIT_FIELD bDirtyBorder : 1;
		// parent drew, therefore this needs to draw, and it's an initial draw.
		BIT_FIELD bParentCleaned : 1;
		// saves it's original surface and restores it before invoking the control draw.
		BIT_FIELD bTransparent : 1; 

		/* Adopted children are not automatically saved in XML files. */
		BIT_FIELD bAdoptedChild : 1;
		// children were cleaned by an internal update... don't draw again.
		BIT_FIELD children_cleaned : 1;
		// no extra init, and no save, this is a support control created for a master control
		BIT_FIELD private_control : 1;
		// control has been temporarily displaced from its parent control.
		BIT_FIELD detached : 1;
		// edit mode enabled visibility of this window and opened it.
		BIT_FIELD auto_opened : 1;
		// first time this is being cleaned (during the course of refresh this could be called many times)
		BIT_FIELD bFirstCleaning : 1;
		// frame was loaded from XML, and desires that EditFrame not be enablable.
		BIT_FIELD bNoEdit : 1;
		// Edit has been enabled on the control.
		BIT_FIELD bEditSet : 1;
		// this came from the XML file.
		BIT_FIELD bEditLoaded : 1;
		// an update event is already being done, bail on this and all children?
		BIT_FIELD bUpdating : 1;
		// enable only real draw events in video thread.  (post invalidate only)
		BIT_FIELD bOpenGL : 1;
		// needs DrawThySelf called... // collect these, so a master level draw can be done down. only if a control or it's child is dirty.
		BIT_FIELD bChildDirty : 1;
		// there is a frame caption update (with flush to display) which needs to be locked....
		BIT_FIELD bRestoring : 1;
		// got at least one frame redraw event (focus happens before first draw)
		BIT_FIELD bShown : 1;
		// Set when resized by a mouse drag, causes a dirty state.
		BIT_FIELD bResizedDirty : 1;
		// during control update the effective surface region was set while away.
		BIT_FIELD bUpdateRegionSet : 1;
		// control was in the process of being cleaned, and received a smudge again... control needs to draw itself AGAIN
		BIT_FIELD bDirtied : 1;
		// parent did a draw, and marks this on all his children, then copy originalsurface clears this flag to indicate it got a clean snapshot
		BIT_FIELD bParentUpdated : 1;
		// this is set on controls which have completed their redraw after a smudge.
		BIT_FIELD bCleanedRecently : 1;
		// this is set by BeginUpdate and EndUpdate to prevent hiding/disable update during update
		BIT_FIELD bDirectUpdating : 1;
		// this is set when BorderType is not the same as the inital border type
		BIT_FIELD bSetBorderType : 1;
		// set when the close button has been added to caption_buttons (standardized)
		BIT_FIELD bCloseButtonAdded : 1;
	} flags;


	/* Information about the caption of a control. */
	/* <combine sack::psi::common_control_frame::caption@1>
		
		\ \                                                  */
	struct {
		/* This is the font that applies to the current control. If it
			is NULL, then it uses the parent controls' font. If that's
			null, it keeps searching up until it finds a font or results
			NULL and uses the default internal font.                     */
		SFTFont font;
		/* This is actually a PFONTDATA.
			                              */
		POINTER fontdata;
		/* Length of the PFONTDATA. */
		size_t fontdatalen;
		/* Text of the control's caption. */
		PTEXT text;
		Image image;
		int pad;  // how much above/below image
	} caption;


	 // original size the control was created at (before the border is applied)
	IMAGE_RECTANGLE original_rect;
	/* the scalex that is applied to creation of controls within
		this control. Modifies the width of a control and X
		positioning.                                              */
	FRACTION scalex;
	/* the scaley that is applied to creation of controls within
		this control. Modifies the height of a control and Y
		positioning.                                              */
	FRACTION scaley;
		// the actual rect of the control...
	IMAGE_RECTANGLE rect;  
	/* This is the rectangle that describes where the surface of the
		control is relative to is outside position.                   */
	IMAGE_RECTANGLE surface_rect;
		// size and position of detachment.
	IMAGE_RECTANGLE detached_at; 
	/* this is the output device that the control is being rendered
		to.                                                          */
	PPHYSICAL_DEVICE device;
	// includes border/caption of control
	Image Window; 
 // actively processing - only when decremented to 0 do we destroy...
	_32 InUse;
	// fake counter to allow ReleaseCommonUse to work.
	_32 NotInUse; 
	// waitinig for a responce... when both inuse and inwait become 0 destroy can happy.
	// otherwise when inuse reduces to 0, draw events are dispatched.
	_32 InWait;
	/* This is a pointer to the prior control in a frame. */
	/* pointer to the first child control in this one. */
	/* pointer to the next control within this control's parent. */
	/* pointer to the control that contains this control. */
	struct common_control_frame *child, *parent, *next, *prior;
	// maybe I can get pointers to this....

	_32 BorderType;
	void (CPROC*BorderDrawProc)( PSI_CONTROL, Image );
	void (CPROC*Rollover)( PSI_CONTROL, LOGICAL );
	void (CPROC*BorderMeasureProc)( PSI_CONTROL, int *x_offset, int *y_offset, int *right_inset, int *bottom_inset );
	// also declare a method above of the same name...
	int draw_result;

	DeclMethod( _DrawThySelf );
	DeclMethod( _DrawDecorations );
	// also declare a method above of the same name...
	DeclMethod( _MouseMethod );
	// also declare a method above of the same name...
	DeclMethod( _KeyProc );
	// also declare a method above of the same name...
	DeclSingleMethod( DrawBorder );
	// also declare a method above of the same name...
	DeclSingleMethod( CaptionChanged );
	// also declare a method above of the same name...
	DeclSingleMethod( Destroy );
	// also declare a method above of the same name...
	DeclSingleMethod( AddedControl );
	// also declare a method above of the same name...
	DeclSingleMethod( ChangeFocus );
	// also declare a method above of the same name...
	DeclSingleMethod( Resize );
	DeclSingleMethod( Move );
	DeclSingleMethod( Rescale );
	//DeclSingleMethod( PosChanging );
	DeclSingleMethod( BeginEdit );
	DeclSingleMethod( EndEdit );
	DeclSingleMethod( DrawCaption );
	DeclMethod( AcceptDroppedFiles );
	/* Pointer to common button data. Common buttons are the Okay
		and Cancel buttons that are commonly on dialogs.           */
	COMMON_BUTTON_DATA pCommonButtonData;
		// invalidating an arbitrary rect, this is the intersection of the parent's dirty rect on this
	IMAGE_RECTANGLE dirty_rect;   
		// during update this may be set, and should be used for the update region insted of control surface
	IMAGE_RECTANGLE update_rect;  
	/* A copy of the name that the frame was loaded from or saved
		to. For subsequent save when the control is edited.        */
	CTEXTSTR save_name;
	CDATA *basecolors;
	/* when registered this gets set as where the control's events and rtti are registered.
		 This will seperate /psi/control and /psi++/control without other flags to switch on */
	PLIST caption_buttons;  // extra controls that are stuffed on the caption bar.
	PFrameBorder border;
	PCAPTION_BUTTON hover_caption_button;  // the current button pressed
	PCAPTION_BUTTON pressed_caption_button;  // the current button pressed
	PCLASSROOT class_root; 
	int nCaptionHeight;
	Image pCaptionImage;
	int nExtra; // size above common required...
} FR_CT_COMMON, *PCONTROL;
//DOM-IGNORE-END

//---------------------------------------------------------------------------

// each control has itself a draw border method.
//void CPROC DrawBorder( PTRSZVAL psv, PSI_CONTROL pc );
// check box uses these... ???
void CPROC DrawThinFrame( PSI_CONTROL pc );
void CPROC DrawThinnerFrame( PSI_CONTROL pc );
void CPROC DrawThinFrameInverted( PSI_CONTROL pc );
void CPROC DrawThinnerFrameInverted( PSI_CONTROL pc );
void CPROC DrawThinFrameImage( Image pc );
/* Draw a 2 pixel frame around a control.
	Parameters
	pc :  COntrol to draw a thinner frame on;. */
void CPROC DrawThinnerFrameImage( PSI_CONTROL pc, Image image );
void CPROC DrawThinFrameInvertedImage( PSI_CONTROL pc, Image image );
void CPROC DrawThinnerFrameInvertedImage( PSI_CONTROL pc, Image image );

void GetCurrentDisplaySurface( PPHYSICAL_DEVICE device );
_MOUSE_NAMESPACE
/* This is an internal routine which sets the hotspot drawing
	coordiantes. Prepares for drawing, but doesn't draw.       */
void SetupHotSpots( PEDIT_STATE pEditState );
/* Routine in mouse space which draws hotspots on the frame
	indicating areas that can be manipulated on a control. Otherwise
	controls are fully active, and you can use them as you are
	developing. Hotspots are drawn in WHITE unless the mouse is
	captured by one, then the spot is RED.
	
	
	Parameters
	pf :  frame being edited.
	pe :  pointer to the current edit state containing information
		   like the currently active control for editing on a frame.  */
void DrawHotSpotsEx( PSI_CONTROL pf, PEDIT_STATE pEditState DBG_PASS );
/* <combine sack::psi::_mouse::DrawHotSpotsEx@PSI_CONTROL@PEDIT_STATE pEditState>
	
	\ \                                                                        */
#define DrawHotSpots(pf,pe) DrawHotSpotsEx(pf,pe DBG_SRC)
//void DrawHotSpots( PSI_CONTROL pf, PEDIT_STATE pEditState );
_MOUSE_NAMESPACE_END
void SmudgeSomeControls( PSI_CONTROL pc, P_IMAGE_RECTANGLE pRect );
void DetachFrameFromRenderer( PSI_CONTROL pc );
void IntelligentFrameUpdateAllDirtyControls( PSI_CONTROL pc DBG_PASS );

_BUTTON_NAMESPACE
//	void InvokeButton( PSI_CONTROL pc );
_BUTTON_NAMESPACE_END

// dir 0 here only... in case we removed ourself from focusable
// dir -1 go backwards
// dir 1 go forwards
#define FFF_HERE      0
#define FFF_FORWARD   1
#define FFF_BACKWARD -1
void FixFrameFocus( PPHYSICAL_DEVICE pf, int dir );

_MOUSE_NAMESPACE
	/* Specifies symbols for which default control to press -
		default accept or default cancel.                      */
	enum MouseInvokeType {
 INV_OKAY   = 0, /* Invoke Button OK. */
 
 INV_CANCEL = 1 /* Invoke Button Cancel. */
 
	};
/* Invokes a default button on a frame.
	Parameters
	pc :    frame to invoke the event on
	type :  type of Event from MouseInvokeType. */
int InvokeDefault( PSI_CONTROL pc, int type );

/* Add a usage counter to a control. Controls in use have redraw
	events blocked.                                               */
void AddUseEx( PSI_CONTROL pc DBG_PASS);
/* <combine sack::psi::_mouse::AddUseEx@PSI_CONTROL pc>
	
	\ \                                              */
#define AddUse( pc ) AddUseEx( pc DBG_SRC )

/* Removes a use added by AddUse. COntrols in use cannot update. */
void DeleteUseEx( PSI_CONTROL *pc DBG_PASS );
/* <combine sack::psi::_mouse::DeleteUseEx@PSI_CONTROL *pc>
	
	\ \                                                  */
#define DeleteUse(pc) DeleteUseEx( &pc DBG_SRC )

/* Adds a wait to a control. This prevents drawing while the
	system is out of a drawable state.                        */
void AddWaitEx( PSI_CONTROL pc DBG_PASS);
/* <combine sack::psi::_mouse::AddWaitEx@PSI_CONTROL pc>
	
	\ \                                               */
#define AddWait( pc ) AddWaitEx( pc DBG_SRC )

/* Removes a wait added by AddWait */
void DeleteWaitEx( PSI_CONTROL *pc DBG_PASS );
#define DeleteWait(pc) DeleteWaitEx( &pc DBG_SRC ) /* <combine sack::psi::_mouse::DeleteWaitEx@PSI_CONTROL *pc>
		                                                
		                                                \ \                                                   */


_MOUSE_NAMESPACE_END
USE_MOUSE_NAMESPACE

int FrameCaptionYOfs( PSI_CONTROL pc, _32 BorderType );

void DrawFrameCaption( PSI_CONTROL pc );

PPHYSICAL_DEVICE OpenPhysicalDevice( PSI_CONTROL pc, PSI_CONTROL over, PRENDERER pActImg, PSI_CONTROL under );
void TryLoadingFrameImage( void );
Image CopyOriginalSurfaceEx( PCONTROL pc, Image use_image DBG_PASS );
#define CopyOriginalSurface(pc,i) CopyOriginalSurfaceEx(pc,i DBG_SRC )

//#define PCOMMON PSI_CONTROL

CDATA *basecolor( PSI_CONTROL pc );

PSI_NAMESPACE_END

#include <controls.h>

#endif

// $Log: controlstruc.h,v $
// Revision 1.73  2005/07/08 00:45:47  d3x0r
// Define NotInUse in control structure.
//
// Revision 1.20  2003/03/25 08:45:56  panther
// Added CVS logging tag
//
