
#include "pages.h"
#ifdef __cplusplus
#  ifndef __NO_ANIMATION__
#    define __NO_ANIMATION__
#  endif
#endif
#ifndef __NO_ANIMATION__
#include "animation.h"
#endif

INTERSHELL_NAMESPACE

struct menu_button_colors
{
	CDATA color;
	CDATA secondary_color; // the border color/ring color... secondary status color...
	CDATA textcolor;
	CDATA highlight_color;
};

struct menu_button
{
	long long x, y, w, h;
	//int nType;

	PLIST colors; // list of struct menu_button_colors *
	struct menu_button_colors base_colors;
	//CDATA color;
	//CDATA secondary_color; // the border color/ring color... secondary status color...
	//CDATA textcolor;
	//CDATA highlight_color;

	TEXTCHAR *text; // button text...
	SFTFont *font_preset;
	CTEXTSTR font_preset_name; // name of the font used on this control...
	TEXTCHAR pImage[256]; // button image (instead of or in addition to text?)
	Image decal_image;
	_32 decal_horiz_margin;
	_32 decal_vert_margin;
#ifndef __NO_ANIMATION__
	char pAnimation[256]; //button animation
	PMNG_ANIMATION  decal_animation;
#endif

	S_16 decal_alpha;
	union {
		PKEY_BUTTON key;
		PSI_CONTROL control;
	}control;
	struct {
		BIT_FIELD bNoPress : 1;

		// if bcustom - no control is owned here, only an abstract handle to something that might be a control
		BIT_FIELD bCustom : 1; // it's not a button, and has it's own control characteristics... (control)
		BIT_FIELD bListbox : 1;  // it's not a button or custom, but is a listbox, which we know some of how to deal with.
		BIT_FIELD bMoved : 1; // sized or dragged to a new position/dimension
		BIT_FIELD bIgnorePageChange : 1;
		BIT_FIELD bSecure : 1;
		BIT_FIELD bSecureEndContext : 1;
		BIT_FIELD bInvisible : 1; // this exists as a macro element or some other way that does not display(allow edit)
		BIT_FIELD bHidden : 1; // well duh, if a thing has said hidden, don't reveal on page show?
		BIT_FIELD bNoCreateMethod : 1; // control did not have a create method found.
		BIT_FIELD bConfigured : 1; // generic configuration was applied to the control; save general parameters, add parsing
		BIT_FIELD bParsingAdded : 1; // just a sanity flag to prevent multiple AddCommonButtonParameter COnfiguration reading.
	} flags;
	PTRSZVAL psvUser;
	TEXTCHAR *pTypeName;
	TEXTCHAR *pPageName; // change to this page after invoking the button's keypress method
	void (CPROC *original_keypress)( PTRSZVAL );
	struct glare_set *glare_set; // glares used on this button

	// if this context is already entered, then the security check is not done.
	/* security is handled by a series of plugin interface methods */
   /* all buttons are potentially securitued, and each is passed to security modules to allow function */
   //TEXTSTR security_context; // once entered, this context is set...
	//TEXTSTR security_reason; // reason to log into permission_user_info
	//TEXTSTR security_token; // filter list of users by these tokens, sepeareted by ','
   //INDEX iSecurityContext; // index into login_history that identifies the context of this login..

	PPAGE_DATA page;
	PLIST show_on; // list of CTEXTSTR's that are system names this button is visible on.
	PLIST no_show_on; // list of CTEXTSTR's that are system names this button is INvisible on.
	//PCanvasData canvas;
	PCanvasData parent_canvas;
	PLIST extra_config; // extra lines that are read from config, (for which do not have a plugin loaded), saved here
	struct menu_button *container_button; // macro elements have this set to their real button, so color updates auto propagate to physical button level.
};

typedef struct menu_button MENU_BUTTON;
INTERSHELL_NAMESPACE_END

