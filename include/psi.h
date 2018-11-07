/*
 * Crafted by: Jim Buckeyne
 *
 * Purpose: Provide a well defined, concise structure for
 *   describing controls.  The only methods that are well
 *   supported are create, destroy, draw, mouse, and keyboard
 *   all others are dispatched as appropriate, but may be
 *   somewhat before or after what others might like.
 *
 *  
 *
 * Define registration function that hooks into deadstart loading
 * is available so that registrations are complete by the time the
 * application starts in main.
 *
 * (c)Freedom Collective, Jim Buckeyne 2006; SACK Collection.
 *
 */



#ifndef PSI_STUFF_DEFINED
#define PSI_STUFF_DEFINED
#include <procreg.h>
#include <controls.h>

PSI_NAMESPACE

#ifdef __cplusplus
#define PSI_ROOT_REGISTRY WIDE("psi++")
#define PSI_ROOT_REGISTRY_OTHER WIDE("psi")
#else
#define PSI_ROOT_REGISTRY WIDE("psi")
#define PSI_ROOT_REGISTRY_OTHER WIDE("psi++")
#endif

#ifdef __cplusplus
//	namespace registration {
#endif
/* Control Registration structure. Obsolete method of
   registering a control and the methods of a control. Internally
   these methods are just registered as they would with the
   procedure registration methods                                 */

struct ControlRegistration_tag {
	/* This is the name of this type of control. Future controls can
	   be created using this name. The name may not contain slashes,
	   but may contain spaces and other punctuation marks.           */
	CTEXTSTR name;
	/* This member is never referenced by name, it defines stuff
	 about the control like default width and height, size of the
	 extra data associated with the control (size of user data),
	 and the default border style of the control.                 */
  	struct control_extra_data {
		/* This member is never referenced by name, it defines stuff
		 about the control like default width and height           */
		struct width_height_tag {
			/* default width of the control. */
			uint32_t width;
			/* default height of the control. */
			uint32_t height;
		}	stuff;
		uint32_t extra; // default width, height for right-click creation.
		uint32_t default_border;  // default border style of the control see BorderOptionTypes
		//CTEXTSTR master_config;
      //struct ControlRegistration_tag *pMasterConfig;
	}
	stuff
	;
	/* <combine sack::PSI::OnCreateCommon>
	   
	   \ \                                 */
	int (CPROC*init)( PSI_CONTROL );
	/* <combine sack::PSI::SetCommonLoad@PSI_CONTROL@void __cdecl*int PSI_CONTROL>
	   
	   This is depricated? Never figured a good way of saving extra
	   data in frames?                                                             */
	int (CPROC*load)( PSI_CONTROL , PTEXT parameters );
	/* <combine sack::PSI::OnDrawCommon>
	   
	   \ \                               */
	int (CPROC*draw)( PSI_CONTROL  );
	/* <combine sack::PSI::OnMouseCommon>
	   
	   \ \                                */
	int (CPROC*mouse)( PSI_CONTROL , int32_t x, int32_t y, uint32_t b );
	/* <combine sack::PSI::OnKeyCommon>
	   
	   \ \                              */
	int (CPROC*key)( PSI_CONTROL , uint32_t );
	/* <combine sack::PSI::OnDestroyCommon>
	   
	   \ \                                  */
	void (CPROC*destroy)( PSI_CONTROL  );
	/* <combine sack::PSI::OnPropertyEdit>
	   
	   \ \                                                        */
	PSI_CONTROL (CPROC*prop_page)( PSI_CONTROL pc );
	/* <combine sack::PSI::OnPropertyEditOkay>
	   
	   \ \                                     */
	void (CPROC*apply_prop)( PSI_CONTROL pc, PSI_CONTROL frame );
	void (CPROC*save)( PSI_CONTROL pc, PVARTEXT pvt );
	void (CPROC*AddedControl)(PSI_CONTROL me, PSI_CONTROL pcAdding );
	void (CPROC*CaptionChanged)( PSI_CONTROL pc );
	/* <combine sack::PSI::OnCommonFocus>
	   
	   \ \                                */
	int (CPROC*FocusChanged)( PSI_CONTROL pc, LOGICAL bFocused );
   /* <combine sack::PSI::OnMotionCommon>
      
      \ \                                 */
   void (CPROC*PositionChanging)( PSI_CONTROL pc, LOGICAL bStart );

	/* \result data - uninitialized this is filled in by the
	   registrar (handler of registration). This Should have been
	   moved up, but it was meant to indicate the end of the
	   registration structure. This type ID is filled in by
	   DoRegisterControl. This type ID is a numeric ID that can be
	   checked to identify the type of the control, and make sure
	   that the user data retreived from the control is the correct
	   type.                                                        */
	uint32_t TypeID; 
};

typedef struct ControlRegistration_tag CONTROL_REGISTRATION;
typedef struct ControlRegistration_tag *PCONTROL_REGISTRATION;

#define LinePaste(a,b) a##b
#define LinePaste2(a,b) LinePaste(a,b)
#define SYMVAL(a) a
#define EasyRegisterControl( name, extra ) static CONTROL_REGISTRATION LinePaste2(ControlType,SYMVAL(__LINE__))= { name, { 32, 32, extra, BORDER_THINNER } }; PRELOAD( SimpleRegisterControl ){ DoRegisterControl( &LinePaste2(ControlType,SYMVAL(__LINE__)) ); } static uint32_t* _MyControlID = &LinePaste2(ControlType,SYMVAL(__LINE__)).TypeID;
#define EasyRegisterControlEx( name, extra, reg_name ) static CONTROL_REGISTRATION reg_name= { name, { 32, 32, extra, BORDER_THINNER } }; PRELOAD( SimpleRegisterControl##reg_name ){ DoRegisterControl( &reg_name ); } static uint32_t* _MyControlID##reg_name = &reg_name.TypeID;
#define EasyRegisterControlWithBorder( name, extra, border_flags ) static CONTROL_REGISTRATION LinePaste2(ControlType,SYMVAL(__LINE__))= { name, { 32, 32, extra, border_flags} }; PRELOAD( SimpleRegisterControl ){ DoRegisterControl( &LinePaste2(ControlType,SYMVAL(__LINE__)) ); } static uint32_t* _MyControlID = &LinePaste2(ControlType,SYMVAL(__LINE__)).TypeID;
#define EasyRegisterControlWithBorderEx( name, extra, border_flags, reg_name ) static CONTROL_REGISTRATION reg_name= { name, { 32, 32, extra, border_flags} }; PRELOAD( SimpleRegisterControl##reg_name ){ DoRegisterControl( &reg_name ); } static uint32_t* _MyControlID##reg_name = &reg_name.TypeID;
#define RegisterControlWithBorderEx( name, extra, border_flags, reg_name )  CONTROL_REGISTRATION reg_name= { name, { 32, 32, extra, border_flags} }; PRELOAD( SimpleRegisterControl##reg_name ){ DoRegisterControl( &reg_name ); } uint32_t* _MyControlID##reg_name = &reg_name.TypeID;
#define ExternRegisterControlWithBorderEx( name, extra, border_flags, reg_name ) extern CONTROL_REGISTRATION reg_name; extern uint32_t* _MyControlID##reg_name;
#define MyControlID (_MyControlID[0])
#define MyControlIDEx(n) (_MyControlID##n[0])
#define MyValidatedControlData( type, result, pc ) ValidatedControlData( type, MyControlID, result, pc )
#define MyValidatedControlDataEx( type, reg_name, result, pc ) ValidatedControlData( type, MyControlIDEx(reg_name), result, pc )
//#define CONTROL_METHODS(draw,mouse,key,destroy) {{"draw",draw},{"mouse",mouse},{"key",key},{"destroy",destroy}}
//---------------------------------------------------------------------------

// please fill out the required forms, submit them once and only once.
// see form definition above, be sure to sign them.
	// in the future (after now), expansion can be handled by observing the size
	// of the registration entry.  at sizeof(registration) - 4 is always the
   // type ID result of this registration...
	PSI_PROC( int, DoRegisterControlEx )( PCONTROL_REGISTRATION pcr, int sizeof_registration );
#define DoRegisterControl(pcr) DoRegisterControlEx( pcr, sizeof(*pcr) )
//PSI_PROC( int, DoRegisterSubcontrol )( PSUBCONTROL_REGISTRATION pcr );

#define ControlData(type,common) ((common)?(*((type*)(common))):NULL)
#define SetControlData(type,common,pf) (*((type*)(common))) = (type)(pf)
#define ValidatedControlData(type,id,result,com) type result = (((com)&&(ControlType(com)==(id)))?ControlData(type,com):NULL)

#ifdef __cplusplus
//using namespace sack::psi::registration;
//namespace methods {
#endif
/* This event is called when a control is hidden. A control that
   is now hidden might do something like stop update timers,
   stop reading input information, or whatever else the control
   is doing that affects its display.
   Example
   <code lang="c++">
   static void OnHideCommon(name)( PSI_CONTROL control )
   {
   
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/hide_common=(PSI_CONTROL)@void@_@hide_common */
#define OnHideCommon(name) \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,HideCommon,WIDE("control"),name WIDE("/hide_control"),PASTE(name,WIDE("hide_control")),void,(PSI_CONTROL),__LINE__)
/* Event given to the control when it is shown. Some controls
   assign new content to themselves if they are now able to be
   shown.
   Example
   <code lang="c#">
   static void OnRevealCommon(name)( PSI_CONTROL control )
   {
       // do something when the control is revealed (was previously hidden)
   }
   </code>
   
   Internal
   Registers under
   
   /psi/control/\<name\>/reveal_control/reveal_control=(PSI_CONTROL)@void@_@reveal_control */
#define OnRevealCommon(name) \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,RevealCommon,WIDE("control"),name WIDE("/reveal_control"),WIDE("reveal_control"),void,(PSI_CONTROL),__LINE__)

/* This is the first event a control will receive. When it is
   created with MakeNamedControl, et al. this event will be
   called. It allows the control to initialize its personal data
   to something. It can be considered a constructor of the
   control.
   Example
   <code lang="c++">
   static int OnCreateCommon(name)( PSI_CONTROL control )
   {
       POINTER user_data = ControlData( control );
   
       //return 0; // return 0 or FALSE to fail control creation.
       return 1; // allow the control to be created
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/rtt/init=(PSI_CONTROL)@int@_@init        */
#define OnCreateCommon(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnCreateCommon,WIDE("control"),name WIDE("/rtti"),WIDE("init"),int,(PSI_CONTROL), __LINE__)
/* Event given to a control when it needs to draw.
   Example
   <code lang="c#">
   static int OnDrawCommon(name)( PSI_CONTROL control )
   {
      // draw (or nto) to a control.
      // return TRUE to update, otherwise FALSE, and update will not be performed.
      // but then, child controls also do not draw, because their parent is not dirty?
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/draw=(PSI_CONTROL)@int@_@draw                           */
#define OnDrawCommon(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnDrawCommon,WIDE("control"),name WIDE("/rtti"),WIDE("draw"),int,(PSI_CONTROL), __LINE__)
/* Event given to a frame control when it needs to draw.
   Example
   <code lang="c#">
   static void OnDrawCaption(name)( PSI_CONTROL control, Image caption )
   {
      // draw (or nto) to a control.
      // return TRUE to update, otherwise FALSE, and update will not be performed.
      // but then, child controls also do not draw, because their parent is not dirty?
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/draw_caption=(PSI_CONTROL,Image)@void@_@draw                           */
#define OnDrawCaption(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnDrawCaption,WIDE("control"),name WIDE("/rtti"),WIDE("draw_caption"),void,(PSI_CONTROL, Image), __LINE__)
/* Event given to a control when it needs to draw its decorations after children have updated.
   Example
   <code lang="c#">
   static void OnDrawCommonDecorations(name)( PSI_CONTROL control )
   {
      // draw decorations (effects after child controls have drawn)
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/draw_decorations=(PSI_CONTROL)@void@_@draw                           */
#define OnDrawCommonDecorations(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnDrawCommonDecorations,WIDE("control"),name WIDE("/rtti"),WIDE("decoration_draw"),void,(PSI_CONTROL), __LINE__)
/* User event callback called when a mouse event happens over a
   control, unless the control has claimed the mouse, in which
   case the mouse may not be over the control. X and Y are
   signed coordinates, if OwnMouse is called, then mouse events
   outside of the control may be trapped. buttons is a
   combination of ButtonFlags. If there is no keyboard event,
   keys will also be given to mouse event as button MK_OBUTTON.
   Example
   <code lang="c++">
   static int OnMouseCommon( TEXT("YourControlName") )( PSI_CONTROL control, int32_t x, int32_t y, uint32_t buttons )
   {
       // if the control uses the mouse, it should return 1, else the mouse is passed through.
       return 0;
   }
   </code>
   
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/mouse =
   (PSI_CONTROL,int32_t,int32_t,uint32_t)@void@_@mouse                                                                */
#define OnMouseCommon(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnMouseCommon,WIDE("control"),name WIDE("/rtti"),WIDE("mouse"),int,(PSI_CONTROL,int32_t,int32_t,uint32_t), __LINE__)
/* Controls may register a keyboard event procedure. This will
   receive notifications about what key is hit. Using mouse keys
   are impractical, because you would have to test every key for
   a new up/down status and figure out which key it was that
   went up and down and whether you should so something. A
   Simple test for like 'is this the space bar' might work as a
   mouse event processing MK_OBUTTON events.
   
   
   Example
   This event may return TRUE if it uses the key or FALSE, and
   the key will be passed to the parent control to see if it
   wants to process it.
   <code lang="c++">
   static int OnKeyCommon( name )( PSI_CONTROL control, uint32_t key )
   {
      // a new key event has happened to this focused control.
      // the key passed may be parsed with macros from keybrd.h
      // that is, a key is a bit packed event described by keybrd.h
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/key=(PSI_CONTROL,uint32_t)@int@_@key       */
#define OnKeyCommon(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnKeyCommon,WIDE("control"),name WIDE("/rtti"),WIDE("key"),int,(PSI_CONTROL,uint32_t), __LINE__)
/* This event callback is called when a control's position
   changes. Usually only happens on the outer parent frame.
   Example
   <code lang="c++">
   static void OnMoveCommon(name)( PSI_CONTROL control, LOGICAL bStart )
   {
       // if bStart is TRUE, the control is about to be moved
       // if bStart is FALSE, then the control is now in a new location.
   }
   </code>
   
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/position_changing=
   (PSI_CONTROL,LOGICAL)@void@_@position_changing                        */
#define OnMoveCommon(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnMoveCommon,WIDE("control"),name WIDE("/rtti"),WIDE("position_changing"),void,(PSI_CONTROL,LOGICAL), __LINE__)
/* User event that is triggered when the size of a control
   changes.
   Example
   The logical parameter passed is TRUE when sizing starts, and
   FALSE when sizing is done.
   <code lang="c++">
   static void OnSizeCommon( "control name" )( PSI_CONTROL control, LOGICAL begin_move )
   {
   
   }
   </code>
   
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/resize =
   (PSI_CONTROL)@void@_@resize                                                           */
#define OnSizeCommon(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnSizeCommon,WIDE("control"),name WIDE("/rtti"),WIDE("resize"),void,(PSI_CONTROL,LOGICAL), __LINE__)
/* User event that is triggered when the size of a control
   changes.
   Example
   The logical parameter passed is TRUE when sizing starts, and
   FALSE when sizing is done.
   <code lang="c++">
   static void OnScaleCommon( "control name" )( PSI_CONTROL control, LOGICAL begin_move )
   {
   
   }
   </code>
   
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/resize =
   (PSI_CONTROL)@void@_@resize                                                           */
#define OnScaleCommon(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnScaleCommon,WIDE("control"),name WIDE("/rtti"),WIDE("rescale"),void,(PSI_CONTROL), __LINE__)

	/* move_starting is TRUE when the position starts changing and
	   is false when the change is done... this allows a critical
	   section to be entered and left around the resize.
	   Example
	   <code lang="c++">
	   static void OnMotionCommon(name)( PSI_CONTROL control, LOGICAL move_starting )
	   {
	       if( move_starting )
	       {
	   </code>
	   <code>
	          // move is starting, but still in old location
	       }
	   </code>
	   <code lang="c++">
	       else
	       {
	          // move has completed and control is in its target location.
	       }
	   }
	   </code>
	   Internal
	   Registers under
	   
	   /psi/control/\<name\>/rtti/some_parents_position_changing=(PSI_CONTROL,LOGICAL)@void@_@some_parents_position_changing */
#define OnMotionCommon(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnMotionCommon,WIDE("control"),name WIDE("/rtti"),WIDE("some_parents_position_changing"),void,(PSI_CONTROL,LOGICAL), __LINE__)

/* Event when a control is being destroyed. Allows a control to
   destroy any internal resources it may have associated with
   the control.
   Example
   <code lang="c++">
   static void OnDestroyCommon(name)( PSI_CONTROL control )
   {
       // do anything you might need when this control is destroyed.
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/destroy=(PSI_CONTROL)@void@_@destroy   */
#define OnDestroyCommon(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnDestroyCommon,WIDE("control"),name WIDE("/rtti"),WIDE("destroy"),void,(PSI_CONTROL), __LINE__)

/* return a frame page to the caller for display.
   Example
   <code lang="c++">
   static PSI_CONTROL OnPropertyEdit(name)( PSI_CONTROL control )
   {
       // return an optional frame which will be added to the
       // edit dialogs tab control.
       return LoadXMLFrameOver(name ".configure.frame", control);
   }
   </code>
   
   return NULL for no dialog.
   Internal
   Registers under
   
   /psi/control/\<name\>/reveal_control/reveal_control=(PSI_CONTROL)@void@_@reveal_control
                                                                                           */
#define OnPropertyEdit(name) \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,PropertyEditControl,WIDE("control"),name WIDE("/rtti"),WIDE("get_property_page"),PSI_CONTROL,(PSI_CONTROL),__LINE__)

	/* bFocused will be true if control gains focus if !bFocused,
	   control is losing focus.
	   
	   Return type is actually void now. The below notes were for
	   before.
	     1. with current focus is told it will lose focus (!bFocus).
	        1. control may allow focus loss (return !FALSE) (goto 3)
	        2. \  control may reject focus loss, which will force it
	           to remain (return FALSE)
	     2. current focus reference of the container is cleared
	     3. newly focused control is told it now has focus.
	        1. it may accept the focus (return TRUE/!FALSE)
	        2. it may reject the focus - and the focus will be left
	           nowhere.
	     4. \  the current focus reference of the container is set
	   Example
	   <code lang="c++">
	   static void OnCommonFocus(name)(PSI_CONTROL control, LOGICAL bFocused )
	   {
	      // update control to show focus.  Buttons draw a line under themselves, list boxes highlight
	      // edit fields draw a cursor for entering text...
	   }
	   </code>
	   
	   Internal
	   Registers under
	   
	   /psi/control/\<name\>/rtti/focus_changed=(PSI_CONTROL,LOGICAL)@void@_@focus_changed             */
#define OnCommonFocus(name) \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,FocusChanged,WIDE("control"),name WIDE("/rtti"),WIDE("focus_changed"),int,(PSI_CONTROL,LOGICAL),__LINE__)

/* The frame edit mode has begun, and controls are given an
   opportunity to make life good for themselves and those around
   them - such as sheet controls displaying sheets separately.
   Example
   <code lang="c++">
   static void OnEditFrame(name)( PSI_CONTROL control )
   {
       // setup control for editing
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/begin_frame_edit=(PSI_CONTROL)@void@_@begin_frame_edit */
#define OnEditFrame(name) \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,FrameEditControl,WIDE("control"),name WIDE("/rtti"),WIDE("begin_frame_edit"),void,(PSI_CONTROL),__LINE__)
/* When a frame's edit phase ends, controls are notified with
   this event.
   Example
   <code lang="c++">
   static void OnEditFrameDone( PSI_CONTROL control )
   {
      // a frame's edit is complete, update control appropriately.
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/end_frame_edit=(PSI_CONTROL)@void@_@end_frame_edit */
#define OnEditFrameDone(name) \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnEditFrameDone,WIDE("control"),name WIDE("/rtti"),WIDE("end_frame_edit"),void,(PSI_CONTROL),__LINE__)
// somet
//#define OnFrameEdit(name)
//	DefineRegistryMethod(PSI_ROOT_REGISTRY,FrameEditControl,"common",name,"begin_edit",void,(void))

/* on okay - read your information for ( your control, from the
   frame )
   Example
   <code lang="c++">
   static void OnPropertyEditOkay(name)( PSI_CONTROL control, PSI_CONTROL property_sheet )
   {
       // read controls on property sheet and update control properties.
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/read_property_page=(PSI_CONTROL,PSI_CONTROL)@void@_@read_property_page */
#define OnPropertyEditOkay(name) \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnPropertyEditOkay,WIDE("control"),name WIDE("/rtti"),WIDE("read_property_page"),void,(PSI_CONTROL,PSI_CONTROL),__LINE__)

/* on cancel return void ( your_control, the sheet your resulted
   to get_property_page
   Example
   <code lang="c++">
   static void OnPropertyEditCancel(name)( PSI_CONTROL control, PSI_CONTROL property_sheet )
   {
      // delete custom property sheet if any.
      // undo any changes as appropriate
   }
   </code>                                                                                   */
#define OnPropertyEditCancel(name) \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,PropertyEditCancelControl,WIDE("control"),name WIDE("/rtti"),WIDE("abort_property_page"),void,(PSI_CONTROL,PSI_CONTROL),__LINE__)

/* some controls may change their appearance and drawing
   characteristics based on having their properties edited. This
   is done after either read or abort is done, also after the
   container dialog is destroyed, thereby indicating that any
   reference to the frame you created is now gone, unless you
   did magic.
   Example
   <code lang="c++">
   static void OnPropertyEditDone(name)( PSI_CONTROL control )
   {
       // do something when edit mode on the frame is being exited.
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/done_property_page=(PSI_CONTROL)@void@_@done_property_page */
#define OnPropertyEditDone( name )  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,PropertyEditDoneControl,WIDE("control"),name WIDE("/rtti"),WIDE("done_property_page"),void,(PSI_CONTROL),__LINE__)

#define OnChangeCaption( name )  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,OnChangeCaption,WIDE("control"),name WIDE("/rtti"),WIDE("caption_changed"),void,(PSI_CONTROL), __LINE__)

#ifndef NO_TOUCH
/* function signature for the touch callback  which can be specified to handle events from touching the display.  see SetMouseHandler.
  would be 'wise' to retun 0 if ignored, 1 if observed (perhaps not used), but NOT ignored.  Return 1 if some of the touches are used.
  This will trigger a check to see if there are unused touches to continue sending... oh but on renderer there's only one callback, more
  important as a note of the control touch event handerer.

   Example
   <code lang="c++">
   static int OnTouchCommon(name)( PSI_CONTROL control, PTOUCHINPUT inputs, int input_count )
   {
       // do something when edit mode on the frame is being exited.
	   // set input[x].dwFlags |= TOUCHEVENTF_USED; as inputs are used.
	   // I also guess only events on that control should be sent, so the input list will change 
	   // by control?
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/touch_event=(PSI_CONTROL,PINPUT_POINT,int)@int@_@touch_event */
#define OnTouchCommon(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnTouchCommon,WIDE("control"),name WIDE("/rtti"),WIDE("touch_event"),int,(PSI_CONTROL,PINPUT_POINT,int), __LINE__)

#endif

/* function signature for the custom border draw routine

   Example
   <code lang="c++">
   static int OnDrawCustomBorder(name)( PSI_CONTROL control, Image window )
	{
	// draw border decoration on the window surface child window can reveal
   // measurements if not known
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/border_draw=(PSI_CONTROL,Image)@void@_@touch_event */
#define OnDrawCustomBorder(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnDrawCustomBorder,WIDE("control"),name WIDE("/rtti"),WIDE("border_draw"),void,(PSI_CONTROL,Image), __LINE__)

		/* function signature for the custom border measure routine.
         allows custom border to specify left/right/top/bottom insets

   Example
   <code lang="c++">
   static int OnMeasuerCustomBorder(name)( PSI_CONTROL control, int *left_inset, int *top_inset, int *right_inset, int *bottom_inset )
	{
     	// fill inset result pointers with correct insets for the control...
   }
   </code>
   Internal
   Registers under
   
   /psi/control/\<name\>/rtti/border_measure=(PSI_CONTROL,int*,int*,int*,int*)@void@_@touch_event */
#define OnMeasureCustomBorder(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnMeasureCustomBorder,WIDE("control"),name WIDE("/rtti"),WIDE("border_measure"),void,(PSI_CONTROL,int*,int*,int*,int*), __LINE__)


// static LOGICAL OnDropAccept(WIDE(""))(PSI_CONTROL pc_canvas,CTEXTSTR filepath,int x,int y)
#define OnControlDropAccept(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnControlDropAccept,WIDE("control"),name WIDE("/rtti"),WIDE("drop_accept"),LOGICAL,(PSI_CONTROL,CTEXTSTR,int32_t,int32_t), __LINE__)

// static void OnControlRollover(WIDE(""))(PSI_CONTROL pc_canvas,LOGICAL enter)
// enter is a boolean if true mouse entered control else mouse left control
#define OnControlRollover(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnControlRollover,WIDE("control"),name WIDE("/rtti"),WIDE("rollover"),void,(PSI_CONTROL,LOGICAL), __LINE__)

// static void OnControlFontChange(WIDE(""))(PSI_CONTROL pc)
// font on control or parent of control has changed.
#define OnControlFontChanged(name)  \
	DefineRegistryMethod(PSI_ROOT_REGISTRY,_OnControlFontChange,WIDE("control"),name WIDE("/rtti"),WIDE("font_change"),void,(PSI_CONTROL), __LINE__)


// just a passing thought.
//#define OnEditFrameBegin( name )
//	DefineRegistryMethod(PSI_ROOT_REGISTRY,EditFrameBegin,"common",name,"frame_edit_begin",void,(PSI_CONTROL))

// and here we can use that fancy declare method thing
// to register the appropriate named things LOL!
//


PSI_NAMESPACE_END

#endif
