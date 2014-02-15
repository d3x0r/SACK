PSI_NAMESPACE
/* A colorwell is a indented control that contains a color.
   Often, if clicked, the color well will show the palette color
   selector to allow the color to be changed. The color may be a
   full alpha representation. When drawn in the well, I think
   the control itself draws the color with 255 alpha.
   
   
   
   Color well controls are "Color Well"
   
   Shade well controls are "Shade Well"
   
   Palette Shader grid is "Color Matrix"
   Example
   <code lang="c++">
   PSI_CONTROL color_well = MakeColorWell( NULL, 5, 5, 25, 25, -1, BASE_COLOR_WHITE );
   </code>
   
   Or using the natrual forms now...
   <code lang="c++">
   PSI_CONTROL shade_well = MakeNamedControl( frame, "Shade Well", 5, 5, 25, 150, -1 );
   SetShadeMin( shade_well, BASE_COLOR_WHITE );
   SetShadeMax( shade_well, BASE_COLOR_RED );
   SetShadeMid( shade_well, BSAE_COLOR_LIGHTBLUE );
   </code>                                                                              */
_COLORWELL_NAMESPACE
	
// common dialog to get a color... returns TRUE if *result is set
// if result is NULL color is not returned, but still can set presets...
#define  COLORWELL_NAME WIDE("Color Well")

/* <combine sack::psi::colorwell::PickColor@CDATA *@CDATA@PSI_CONTROL>
   
   \ \ 
   Parameters
   x :  position of the color choice dialog, otherwise uses the
        mouse position.
   y :  position of the color choice dialog, otherwise uses the
        mouse position.                                                */
PSI_PROC( int, PickColorEx )( CDATA *result, CDATA original, PSI_CONTROL hAbove, int x, int y );
/* Shows a color picking dialog. Results with a chosen color.
   Parameters
   result\ :   pointer to CDATA to result into
   original :  CDATA that is the color to start from.
   pAbove :    frame to stack this dialog over.
   
   Returns
   TRUE if *result is set, else FALSE. (Reflects the Ok/Cancel
   of the dialog)                                              */
PSI_PROC( int, PickColor)( CDATA *result, CDATA original, PSI_CONTROL pAbove );
// creates a control which can then select a color

CONTROL_PROC( ColorWell, (CDATA color) );
/* Macro to create a colorwell.
   Parameters
   f :   frame to create the control in
   x :   x coordinate of the left of the control in the frame
   y :   y coordinate of the left of the control in the frame
   w :   width of the control
   h :   height of the control
   ID :  an integer identifier for the control
   c :   a CDATA color to initialize with.
   
   Returns
   \Returns a PSI_CONTROL that is a color well.               */
#define MakeColorWell(f,x,y,w,h,id,c) SetColorWell( MakeNamedControl(f,COLORWELL_NAME,x,y,w,h,id ),c)

/* Three colors define the gradient in a shade well. They can be
   all the same and show a solid color, but this is for picking
   colors from linear gradients. The 'Min' is one end, the 'Mid'
   is the center and 'Max' is the other end. Control also only
   does this vertically.
   
   
   Parameters
   pc :     "Shade Well" control
   color :  Color to set the max end to.                         */
PSI_PROC( void, SetShadeMin )( PSI_CONTROL pc, CDATA color );
/* Three colors define the gradient in a shade well. They can be
   all the same and show a solid color, but this is for picking
   colors from linear gradients. The 'Min' is one end, the 'Mid'
   is the center and 'Max' is the other end. Control also only
   does this vertically.
   Parameters
   pc :     Shade Well control
   color :  color to set the max to.                             */
PSI_PROC( void, SetShadeMax )( PSI_CONTROL pc, CDATA color );
/* Three colors define the gradient in a shade well. They can be
   all the same and show a solid color, but this is for picking
   colors from linear gradients. The 'Min' is one end, the 'Mid'
   is the center and 'Max' is the other end. Control also only
   does this vertically.
   Parameters
   pc :     "Shade Well" control
   color :  sets the mid color of the control                    */
PSI_PROC( void, SetShadeMid )( PSI_CONTROL pc, CDATA color );

/* Sets the current color of a "Color Well"
   Parameters
   pc :     a "Color Well" control
   color :  CDATA to set the color to.      */
PSI_PROC( PSI_CONTROL, SetColorWell )( PSI_CONTROL pc, CDATA color );
/* Enables clicking in a color well to auto show the dialog.
   Parameters
   pc :       "Color Well" control to enable auto pick
   bEnable :  If TRUE, enable autopick; if FALSE, disable autopick. */
PSI_PROC( PSI_CONTROL, EnableColorWellPick )( PSI_CONTROL pc, LOGICAL bEnable );
/* Sets the handler for when the control is clicked. (or when
   it's changed?)
   Parameters
   PC :             pointer to a "Color Well" 
   EventCallback :  This routine is called when the color is
                    changed.
   user_data :      user data to be passed to the callback when
                    invoked.                                    */
PSI_PROC( PSI_CONTROL, SetOnUpdateColorWell )( PSI_CONTROL PC, void(CPROC*EventCallback)(PTRSZVAL,CDATA), PTRSZVAL user_data);
/* Gets the CDATA color in a color well.
   Parameters
   pc :  "Color Well" control to get the color from */
PSI_PROC( CDATA, GetColorFromWell )( PSI_CONTROL pc );
PSI_COLORWELL_NAMESPACE_END
USE_PSI_COLORWELL_NAMESPACE


