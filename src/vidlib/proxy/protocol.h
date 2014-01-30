
#ifdef _MSC_VER
#pragma pack (push, 1)
#endif
#ifndef UNALIGNED
#define UNALIGNED
#endif

#define MSGBLOCK(type,...) struct commsg_##type { __VA_ARGS__ } type
PREFIX_PACKED struct opendisplay_data 
{
	S_32 x, y;
	_32 w, h;
	_32 attr;
	PTRSZVAL server_display_id;
	PTRSZVAL over;
	PTRSZVAL under;
} PACKED;

PREFIX_PACKED struct make_image_data 
{
	_32 w, h;
	// what the server calls this image; for all further draw ops
	PTRSZVAL server_image_id;
	// so the client can know which to output surface attach to
	PTRSZVAL server_display_id;
} PACKED;

PREFIX_PACKED struct image_data_data 
{
	// what the server calls this image; for all further draw ops
	PTRSZVAL server_image_id;
	// so the client can know which to output surface attach to
	TEXTCHAR data[1];
} PACKED;

PREFIX_PACKED struct make_subimage_data 
{
	S_32 x, y;
	_32 w, h;
	PTRSZVAL server_parent_image_id;
	PTRSZVAL server_image_id;
} PACKED;

PREFIX_PACKED struct __tmp
{
	_32 data;
} PACKED;

PREFIX_PACKED struct blatcolor_data
{
	PTRSZVAL server_image_id;
	S_32 x, y;
	_32 w, h;
	CDATA color;
} PACKED;

PREFIX_PACKED struct blot_image_data
{
	PTRSZVAL server_image_id;
	S_32 x, y;
	_32 w, h;
	S_32 xs, ys;
	PTRSZVAL image_id;
} PACKED;

PREFIX_PACKED struct blot_scaled_image_data
{
	PTRSZVAL server_image_id;
	S_32 x, y;
	_32 w, h;
	S_32 xs, ys;
	_32 ws, hs;
	PTRSZVAL image_id;
} PACKED;

PREFIX_PACKED struct common_message {
	_8 message_id;
	union
	{
		TEXTCHAR UNALIGNED text[1];  // actually is more than one
		MSGBLOCK( version,
					_8 bits;
					 _8 unicode;
					 _8 number; );
		struct opendisplay_data opendisplay_data;
		struct blatcolor_data blatcolor;
		struct make_image_data make_image;
		struct make_subimage_data make_subimage;
		struct image_data_data image_data;
		struct blot_image_data blot_image;
		struct blot_scaled_image_data blot_scaled_image;
		MSGBLOCK( open_display_reply,  PTRSZVAL server_display_id; PTRSZVAL client_display_id; );
	} data;
} PACKED;

#ifdef _MSC_VER
#pragma pack (pop)
#endif

enum proxy_message_id{
	     PMID_Version   // 0
							, PMID_SetApplicationTitle   //1
							, PMID_SetApplicationIcon  // 2

							, PMID_OpenDisplayAboveUnderSizedAt  // 3
							, PMID_CloseDisplay  // 4
							, PMID_Reply_OpenDisplayAboveUnderSizedAt  // 5

							, PMID_MakeImage // 6
							, PMID_MakeSubImage // 7
							
							, PMID_BlatColor // 8
							, PMID_BlatColorAlpha // 9 
							, PMID_ImageData // 10 - transfer local image data to client
							, PMID_BlotImageSizedTo  // 11 
							, PMID_BlotScaledImageSizedTo // 12

#if 0
    RENDER_PROC_PTR( void, UpdateDisplayPortionEx) ( PRENDERER, S_32 x, S_32 y, _32 width, _32 height DBG_PASS );
    /* <combine sack::image::render::UpdateDisplayEx@PRENDERER>
       
       \ \                                                      */
    RENDER_PROC_PTR( void, UpdateDisplayEx)        ( PRENDERER DBG_PASS);
                             
                                                   
                                                   \ \                                                   */
   
    /* <combine sack::image::render::GetDisplayPosition@PRENDERER@S_32 *@S_32 *@_32 *@_32 *>
       
       \ \                                                                                   */
    RENDER_PROC_PTR( void, GetDisplayPosition)   ( PRENDERER, S_32 *x, S_32 *y, _32 *width, _32 *height );
    /* <combine sack::image::render::MoveDisplay@PRENDERER@S_32@S_32>
       
       \ \                                                            */
    RENDER_PROC_PTR( void, MoveDisplay)          ( PRENDERER, S_32 x, S_32 y );
    /* <combine sack::image::render::MoveDisplayRel@PRENDERER@S_32@S_32>
       
       \ \                                                               */
    RENDER_PROC_PTR( void, MoveDisplayRel)       ( PRENDERER, S_32 delx, S_32 dely );
    /* <combine sack::image::render::SizeDisplay@PRENDERER@_32@_32>
       
       \ \                                                          */
    RENDER_PROC_PTR( void, SizeDisplay)          ( PRENDERER, _32 w, _32 h );
    /* <combine sack::image::render::SizeDisplayRel@PRENDERER@S_32@S_32>
       
       \ \                                                               */
    RENDER_PROC_PTR( void, SizeDisplayRel)       ( PRENDERER, S_32 delw, S_32 delh );
    /* <combine sack::image::render::MoveSizeDisplayRel@PRENDERER@S_32@S_32@S_32@S_32>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, MoveSizeDisplayRel )  ( PRENDERER hVideo
                                                 , S_32 delx, S_32 dely
                                                 , S_32 delw, S_32 delh );
    RENDER_PROC_PTR( void, PutDisplayAbove)      ( PRENDERER, PRENDERER ); /* <combine sack::image::render::PutDisplayAbove@PRENDERER@PRENDERER>
                                                              
                                                              \ \                                                                */
 
    /* <combine sack::image::render::GetDisplayImage@PRENDERER>
       
       \ \                                                      */
    RENDER_PROC_PTR( Image, GetDisplayImage)     ( PRENDERER );

    /* <combine sack::image::render::SetCloseHandler@PRENDERER@CloseCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetCloseHandler)      ( PRENDERER, CloseCallback, PTRSZVAL );
    /* <combine sack::image::render::SetMouseHandler@PRENDERER@MouseCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetMouseHandler)      ( PRENDERER, MouseCallback, PTRSZVAL );
    /* <combine sack::image::render::SetRedrawHandler@PRENDERER@RedrawCallback@PTRSZVAL>
       
       \ \                                                                               */
    RENDER_PROC_PTR( void, SetRedrawHandler)     ( PRENDERER, RedrawCallback, PTRSZVAL );
    /* <combine sack::image::render::SetKeyboardHandler@PRENDERER@KeyProc@PTRSZVAL>
       
       \ \                                                                          */
    RENDER_PROC_PTR( void, SetKeyboardHandler)   ( PRENDERER, KeyProc, PTRSZVAL );
    /* <combine sack::image::render::SetLoseFocusHandler@PRENDERER@LoseFocusCallback@PTRSZVAL>
       
       \ \                                                                                     */
    RENDER_PROC_PTR( void, SetLoseFocusHandler)  ( PRENDERER, LoseFocusCallback, PTRSZVAL );
    /* <combine sack::image::render::SetDefaultHandler@PRENDERER@GeneralCallback@PTRSZVAL>
       
       \ \                                                                                 */
#if ACTIVE_MESSAGE_IMPLEMENTED
			 RENDER_PROC_PTR( void, SetDefaultHandler)    ( PRENDERER, GeneralCallback, PTRSZVAL );
#else
       POINTER junk1;
#endif
    /* <combine sack::image::render::GetMousePosition@S_32 *@S_32 *>
       
		 \ \                                                           */
    RENDER_PROC_PTR( void, GetMousePosition)     ( S_32 *x, S_32 *y );
    /* <combine sack::image::render::SetMousePosition@PRENDERER@S_32@S_32>
       
       \ \                                                                 */
    RENDER_PROC_PTR( void, SetMousePosition)     ( PRENDERER, S_32 x, S_32 y );

    /* <combine sack::image::render::HasFocus@PRENDERER>
       
       \ \                                               */
    RENDER_PROC_PTR( LOGICAL, HasFocus)          ( PRENDERER );

#if ACTIVE_MESSAGE_IMPLEMENTED
    RENDER_PROC_PTR( int, SendActiveMessage)     ( PRENDERER dest, PACTIVEMESSAGE msg );
    RENDER_PROC_PTR( PACTIVEMESSAGE , CreateActiveMessage) ( int ID, int size, ... );
#else
	 /* Just a place holder value. Some function used to be here. */
	 POINTER junk2;
	 /* Place Holder value - depricated functions in interface. */
	 POINTER junk3;
#endif
    /* <combine sack::image::render::GetKeyText@int>
       
       \ \                                           */
    RENDER_PROC_PTR( TEXTCHAR, GetKeyText)           ( int key );
    /* <combine sack::image::render::IsKeyDown@PRENDERER@int>
       
       \ \                                                    */
    RENDER_PROC_PTR( _32, IsKeyDown )        ( PRENDERER display, int key );
    /* <combine sack::image::render::KeyDown@PRENDERER@int>
       
       \ \                                                  */
    RENDER_PROC_PTR( _32, KeyDown )         ( PRENDERER display, int key );
    /* <combine sack::image::render::DisplayIsValid@PRENDERER>
       
       \ \                                                     */
    RENDER_PROC_PTR( LOGICAL, DisplayIsValid )  ( PRENDERER display );
    /* <combine sack::image::render::OwnMouseEx@PRENDERER@_32 bOwn>
       
       \ \                                                          */
    RENDER_PROC_PTR( void, OwnMouseEx )            ( PRENDERER display, _32 Own DBG_PASS);
    /* <combine sack::image::render::BeginCalibration@_32>
       
       \ \                                                 */
    RENDER_PROC_PTR( int, BeginCalibration )       ( _32 points );
    /* <combine sack::image::render::SyncRender@PRENDERER>
       
       \ \                                                 */
    RENDER_PROC_PTR( void, SyncRender )            ( PRENDERER pDisplay );
    /* DEPRICATED; left in structure for compatibility.  Removed define and export definition. */
    RENDER_PROC_PTR( int, EnableOpenGL )           ( PRENDERER hVideo );
    /* DEPRICATED; left in structure for compatibility.  Removed define and export definition. */
    RENDER_PROC_PTR( int, SetActiveGLDisplay )     ( PRENDERER hDisplay );

	 /* <combine sack::image::render::MoveSizeDisplay@PRENDERER@S_32@S_32@S_32@S_32>
	    
	    \ \                                                                          */
	 RENDER_PROC_PTR( void, MoveSizeDisplay )( PRENDERER hVideo
                                        , S_32 x, S_32 y
                                        , S_32 w, S_32 h );
   /* <combine sack::image::render::MakeTopmost@PRENDERER>
      
      \ \                                                  */
   RENDER_PROC_PTR( void, MakeTopmost )    ( PRENDERER hVideo );
   /* <combine sack::image::render::HideDisplay@PRENDERER>
      
      \ \                                                  */
   RENDER_PROC_PTR( void, HideDisplay )      ( PRENDERER hVideo );
   /* <combine sack::image::render::RestoreDisplay@PRENDERER>
      
      \ \                                                     */
   RENDER_PROC_PTR( void, RestoreDisplay )   ( PRENDERER hVideo );

	/* <combine sack::image::render::ForceDisplayFocus@PRENDERER>
	   
	   \ \                                                        */
	RENDER_PROC_PTR( void, ForceDisplayFocus )( PRENDERER display );
	/* <combine sack::image::render::ForceDisplayFront@PRENDERER>
	   
	   \ \                                                        */
	RENDER_PROC_PTR( void, ForceDisplayFront )( PRENDERER display );
	/* <combine sack::image::render::ForceDisplayBack@PRENDERER>
	   
	   \ \                                                       */
	RENDER_PROC_PTR( void, ForceDisplayBack )( PRENDERER display );

	/* <combine sack::image::render::BindEventToKey@PRENDERER@_32@_32@KeyTriggerHandler@PTRSZVAL>
	   
	   \ \                                                                                        */
	RENDER_PROC_PTR( int, BindEventToKey )( PRENDERER pRenderer, _32 scancode, _32 modifier, KeyTriggerHandler trigger, PTRSZVAL psv );
	/* <combine sack::image::render::UnbindKey@PRENDERER@_32@_32>
	   
	   \ \                                                        */
	RENDER_PROC_PTR( int, UnbindKey )( PRENDERER pRenderer, _32 scancode, _32 modifier );
	/* <combine sack::image::render::IsTopmost@PRENDERER>
	   
	   \ \                                                */
	RENDER_PROC_PTR( int, IsTopmost )( PRENDERER hVideo );
	/* Used as a point to sync between applications and the message
	   display server; Makes sure that all draw commands which do
	   not have a response are done.
	   
	   
	   
	   Waits until all commands are processed; which is wait until
	   this command is processed.                                   */
	RENDER_PROC_PTR( void, OkaySyncRender )            ( void );
   /* <combine sack::image::render::IsTouchDisplay>
      
      \ \                                           */
   RENDER_PROC_PTR( int, IsTouchDisplay )( void );
	/* <combine sack::image::render::GetMouseState@S_32 *@S_32 *@_32 *>
	   
	   \ \                                                              */
	RENDER_PROC_PTR( void , GetMouseState )        ( S_32 *x, S_32 *y, _32 *b );
	/* <combine sack::image::render::EnableSpriteMethod@PRENDERER@void__cdecl*RenderSpritesPTRSZVAL psv\, PRENDERER renderer\, S_32 x\, S_32 y\, _32 w\, _32 h@PTRSZVAL>
	   
	   \ \                                                                                                                                                               */
	RENDER_PROC_PTR ( PSPRITE_METHOD, EnableSpriteMethod )(PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv );
	/* <combine sack::image::render::WinShell_AcceptDroppedFiles@PRENDERER@dropped_file_acceptor@PTRSZVAL>
	   
	   \ \                                                                                                 */
	RENDER_PROC_PTR( void, WinShell_AcceptDroppedFiles )( PRENDERER renderer, dropped_file_acceptor f, PTRSZVAL psvUser );
	/* <combine sack::image::render::PutDisplayIn@PRENDERER@PRENDERER>
	   
	   \ \                                                             */
	RENDER_PROC_PTR(void, PutDisplayIn) (PRENDERER hVideo, PRENDERER hContainer);
#ifdef WIN32
	/* <combine sack::image::render::MakeDisplayFrom@HWND>
	   
	   \ \                                                 */
			RENDER_PROC_PTR (PRENDERER, MakeDisplayFrom) (HWND hWnd) ;
#else
      POINTER junk4;
#endif
	/* <combine sack::image::render::SetRendererTitle@PRENDERER@TEXTCHAR *>
	   
	   \ \                                                                  */
	RENDER_PROC_PTR( void , SetRendererTitle) ( PRENDERER render, const TEXTCHAR *title );
	/* <combine sack::image::render::DisableMouseOnIdle@PRENDERER@LOGICAL>
	   
	   \ \                                                                 */
	RENDER_PROC_PTR (void, DisableMouseOnIdle) (PRENDERER hVideo, LOGICAL bEnable );
	/* <combine sack::image::render::OpenDisplayAboveUnderSizedAt@_32@_32@_32@S_32@S_32@PRENDERER@PRENDERER>
	   
	   \ \                                                                                                   */
	RENDER_PROC_PTR( PRENDERER, OpenDisplayAboveUnderSizedAt)( _32 attributes, _32 width, _32 height, S_32 x, S_32 y, PRENDERER above, PRENDERER under );
	/* <combine sack::image::render::SetDisplayNoMouse@PRENDERER@int>
	   
	   \ \                                                            */
	RENDER_PROC_PTR( void, SetDisplayNoMouse )( PRENDERER hVideo, int bNoMouse );
	/* <combine sack::image::render::Redraw@PRENDERER>
	   
	   \ \                                             */
	RENDER_PROC_PTR( void, Redraw )( PRENDERER hVideo );
	/* <combine sack::image::render::MakeAbsoluteTopmost@PRENDERER>
	   
	   \ \                                                          */
	RENDER_PROC_PTR(void, MakeAbsoluteTopmost) (PRENDERER hVideo);
	/* <combine sack::image::render::SetDisplayFade@PRENDERER@int>
	   
	   \ \                                                         */
	RENDER_PROC_PTR( void, SetDisplayFade )( PRENDERER hVideo, int level );
	/* <combine sack::image::render::IsDisplayHidden@PRENDERER>
	   
	   \ \                                                      */
	RENDER_PROC_PTR( LOGICAL, IsDisplayHidden )( PRENDERER video );
#ifdef WIN32
	/* <combine sack::image::render::GetNativeHandle@PRENDERER>
	   
	   \ \                                                      */
	RENDER_PROC_PTR( HWND, GetNativeHandle )( PRENDERER video );
#endif
		 /* <combine sack::image::render::GetDisplaySizeEx@int@S_32 *@S_32 *@_32 *@_32 *>
		    
		    \ \                                                                           */
		 RENDER_PROC_PTR (void, GetDisplaySizeEx) ( int nDisplay
														  , S_32 *x, S_32 *y
														  , _32 *width, _32 *height);

	/* Locks a video display. Applications shouldn't be locking
	   this, but if for some reason they require it; use this
	   function.                                                */
	RENDER_PROC_PTR( void, LockRenderer )( PRENDERER render );
	/* Release renderer lock critical section. Applications
	   shouldn't be locking this surface.                   */
	RENDER_PROC_PTR( void, UnlockRenderer )( PRENDERER render );
	/* Provides a way for applications to cause the window to flush
	   to the display (if it's a transparent window)                */
	RENDER_PROC_PTR( void, IssueUpdateLayeredEx )( PRENDERER hVideo, LOGICAL bContent, S_32 x, S_32 y, _32 w, _32 h DBG_PASS );
	/* Check to see if the render mode is always redraw; changes how
	   smudge works in PSI. If always redrawn, then the redraw isn't
	   done during the smudge, and instead is delayed until a draw
	   is triggered at which time all controls are drawn.
	   
	   
	   
	   
	   Returns
	   TRUE if full screen needs to be drawn during a draw,
	   otherwise partial updates may be done.                        */
	RENDER_PROC_PTR( LOGICAL, RequiresDrawAll )( void );
#ifndef NO_TOUCH
		/* <combine sack::image::render::SetTouchHandler@PRENDERER@fte inc asdfl;kj
		 fteTouchCallback@PTRSZVAL>
       
       \ \                                                                             */
			RENDER_PROC_PTR( void, SetTouchHandler)      ( PRENDERER, TouchCallback, PTRSZVAL );
#endif
    RENDER_PROC_PTR( void, MarkDisplayUpdated )( PRENDERER );
    /* <combine sack::image::render::SetHideHandler@PRENDERER@HideAndRestoreCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetHideHandler)      ( PRENDERER, HideAndRestoreCallback, PTRSZVAL );
    /* <combine sack::image::render::SetRestoreHandler@PRENDERER@HideAndRestoreCallback@PTRSZVAL>
       
       \ \                                                                             */
    RENDER_PROC_PTR( void, SetRestoreHandler)      ( PRENDERER, HideAndRestoreCallback, PTRSZVAL );
		 RENDER_PROC_PTR( void, RestoreDisplayEx )   ( PRENDERER hVideo DBG_PASS );
		 /* added for android extensions; call to enable showing the keyboard in the correct thread
        ; may have applications for windows tablets 
		  */
       RENDER_PROC_PTR( void, SACK_Vidlib_ShowInputDevice )( void );
		 /* added for android extensions; call to enable hiding the keyboard in the correct thread
		  ; may have applications for windows tablets */
       RENDER_PROC_PTR( void, SACK_Vidlib_HideInputDevice )( void );

#endif
							, PMID_LAST_PROXY_MESSAGE

							//};

							//enum proxy_message_id_3d{
							, PMID_GetRenderTransform = PMID_LAST_PROXY_MESSAGE
							, PMID_ClipPoints
							, PMID_GetViewVolume

						, PMID_MakeImageFile
						, PMID_MakeSubImageFile
						, PMID_LoadImageFileFromGroup
						, PMID_UnmakeImageFile
};
