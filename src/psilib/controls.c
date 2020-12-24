#include <stdhdrs.h>
#include <sqlgetoption.h>
#include <sharemem.h>
#include <logging.h>
#include <idle.h>
#include <timers.h>
#include <filesys.h>
#include <fractions.h>
#include <interface.h>
#include <procreg.h>
#define CONTROL_BASE
#define LOCK_TEST 0
// this is a FUN flag! this turns on
// background state capture for all controls...
// builds in the required function of get/restore
// background image before dispatching draw events.
//#define DEFAULT_CONTROLS_TRANSPARENT

//#define DEBUG_FOCUS_STUFF

/* this flag is defined in controlstruc.h...
 *
 * #define DEBUG_BORDER_DRAWING
 *
 */

//#define DEBUG_CREATE
//#define DEBUG_SCALING
// this symbol is also used in XML_Load code.
//#define DEBUG_RESOURCE_NAME_LOOKUP
//#define DEBUG_TRANSPARENCY_SURFACE_SAVE_RESTORE
#define DEBUG_UPDAATE_DRAW 4

// defined to use the new interface manager.
// otherwise this library had to do twisted steps
// to load what it thinks it wants for interfaces,
// and was not externally configurable.
#ifndef FORCE_NO_INTERFACE
#define USE_INTERFACES
#endif

/* this might be defined on Linux, and or on non vista platforms? */
//#ifdef __LINUX__
/* this is an important option to leave ON.  with current generation (2007-06-06) code,
 * the control's surface is what is mostly what is concentrated on for refresh.
 * border drawing was minimized a hair too far... but without this, borders are draw and redrawn
 * hundreds of time that are unnessecary.  But, this cures some of those artirfacts...
 * it should someday not be required... as the frame's surface should be a static state
 */
//#define SMUDGE_ON_VIDEO_UPDATE
//#endif


//#define BLAT_COLOR_UPDATE_PORTION
/*
 *
 * (4) most logging (so far) this has a level really noisy messages are #if DEBUG_UPDATE > 3
 * (3) has control update path, not only just when updates occur
 *
 */

#include "controlstruc.h"
#include <keybrd.h>
#include <psi.h>
#include "mouse.h"
#include "borders.h"

PSI_NAMESPACE

#include "resource.h"

#ifdef BLAT_COLOR_UPDATE_PORTION
// was for testing blotting regions...
  CDATA TESTCOLOR;
#endif
typedef struct resource_names
{
	uint32_t resource_name_id;
	uint32_t resource_name_range;
	CTEXTSTR resource_name;
	CTEXTSTR type_name;
} RESOURCE_NAMES;
static RESOURCE_NAMES resource_names[] = {
#define BUILD_NAMES
#ifdef __cplusplus
#define FIRST_SYMNAME(name,control_type_name)  { name, 1, #name, control_type_name }
#define SYMNAME(name,control_type_name)  , { name, 1, #name, control_type_name }
#define SYMNAME_SKIP(prior,range,name,control_type_name)  , { prior, range, #prior, control_type_name } \
	, { name, 1, #name, control_type_name }
#else
#ifdef __WATCOMC__
#define FIRST_SYMNAME(name,control_type_name)  [name - FIRST_SYMBOL] = { name, 1, #name, control_type_name }
#define SYMNAME(name,control_type_name)  , [name - FIRST_SYMBOL] = { name, 1, #name, control_type_name }
#define SYMNAME_SKIP(prior,range,name,control_type_name)  , [prior - FIRST_SYMBOL] = { prior, range, #prior, control_type_name } \
	, [name - FIRST_SYMBOL] = { name, 1, #name, control_type_name }
#else
#define FIRST_SYMNAME(name,control_type_name)  { name, 1, #name, control_type_name }
#define SYMNAME(name,control_type_name)  , { name, 1, #name, control_type_name }
#define SYMNAME_SKIP(prior,range,name,control_type_name)  , { prior, range, #prior, control_type_name } \
	, { name, 1, #name, control_type_name }
#endif
#endif
#include "resource.h"
};

TEXTCHAR *GetResourceIDName( CTEXTSTR pTypeName, int ID )
{
	PCLASSROOT data = NULL;
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( PSI_ROOT_REGISTRY "/resources/", &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		TEXTCHAR rootname[256];
		CTEXTSTR name2;
		PCLASSROOT data2 = NULL;
		tnprintf( rootname, sizeof(rootname),PSI_ROOT_REGISTRY "/resources/%s/%s", name, pTypeName );
		//lprintf( "newroot = %s", rootname );
		for( name2 = GetFirstRegisteredName( rootname, &data2 );
			 name2;
			  name2 = GetNextRegisteredName( &data2 ) )
		{
			int value = (int)(uintptr_t)(GetRegisteredValueExx( data2, name2, "value", TRUE ));
			int range = (int)(uintptr_t)(GetRegisteredValueExx( data2, name2, "range", TRUE ));
			//lprintf( "Found Name %s", name2 );
			if( (value <= ID) && ((value+range) > ID) )
			{
				size_t len;
				TEXTCHAR *result = NewArray( TEXTCHAR, len = strlen( pTypeName ) + strlen( name ) + strlen( name2 ) + 3 + 20 );
				if( value == ID )
					tnprintf( result, len*sizeof(TEXTCHAR), "%s/%s/%s", name, pTypeName, name2 );
				else
					tnprintf( result, len*sizeof(TEXTCHAR), "%s/%s/%s+%d", name, pTypeName, name2, ID-value );
				return result;
			}
		}
	}
	return NULL;
}


// also fix the name passed in?
int GetResourceID( PSI_CONTROL parent, CTEXTSTR name, uint32_t nIDDefault )
{
	if( !pathchr( name ) )
	{
		// assume search mode to find name... using name as terminal leef, but application and control class are omitted...
		PCLASSROOT data = NULL;
		CTEXTSTR name;
		//DebugBreak(); // changed 'name' to data and 'name2' to 'data2' ...
		for( name = GetFirstRegisteredName( PSI_ROOT_REGISTRY "/resources/", &data );
			 name;
			  name = GetNextRegisteredName( &data ) )
		{
			//TEXTCHAR rootname[256];
			CTEXTSTR name2;
			PCLASSROOT data2 = NULL;
			//tnprintf( rootname, sizeof(rootname),PSI_ROOT_REGISTRY "/resources/%s", name );
			//lprintf( "newroot = %s", rootname );
			for( name2 = GetFirstRegisteredName( (CTEXTSTR)data, &data2 );
				 name2;
				  name2 = GetNextRegisteredName( &data2 ) )
			{
				TEXTCHAR rootname[256];
				int nResult;
				//lprintf( "Found Name %s", name2 );
				tnprintf( rootname, sizeof( rootname ), PSI_ROOT_REGISTRY "/resources/%s/%s", name, name2 );
				if( GetRegisteredStaticIntValue( NULL, rootname, name, &nResult ) )
				{
					return nResult;
				}
				else
				{
					if( ( nIDDefault != -1 ) )
					{
						RegisterIntValue( rootname, "value", nIDDefault );
						//RegisterIntValue( rootname, "range", offset + 1 );
						return nIDDefault;
					}
				}
			}
		}
	}
	else
	{
		CTEXTSTR ofs_string;
		int offset = 0;
		ofs_string = strchr( name, '+' );
		if( ofs_string )
		{
			offset = (int)IntCreateFromText( ofs_string );
			(*((TEXTCHAR*)ofs_string)) = 0;
		}
		{
			int result;
			int range;
			TEXTCHAR buffer[256];
			tnprintf( buffer, sizeof( buffer ), PSI_ROOT_REGISTRY "/resources/%s", name );
			range = (int)(uintptr_t)GetRegisteredValueExx( (PCLASSROOT)PSI_ROOT_REGISTRY "/resources", name, "range", TRUE );
			result = (int)(uintptr_t)GetRegisteredValueExx( (PCLASSROOT)PSI_ROOT_REGISTRY "/resources", name, "value", TRUE )/* + offset*/;
			if( !result && !range && ( nIDDefault != -1 ) )
			{
				RegisterIntValue( buffer, "value", nIDDefault );
				RegisterIntValue( buffer, "range", offset + 1 );
				result = nIDDefault;
				range = offset + 1;
			}
			// auto offset old resources...
			// this is probably depricated code, but wtf.
			if( parent && range > 1 && !ofs_string )
			{
				PSI_CONTROL pc;
				offset = 0; // alternative way to compute offset here...
				for( pc = parent->child; pc; pc = pc->next )
				{
					if( pc->nID >= result && ( pc->nID < (result+range) ) )
						offset++;
				}
			}
			result += offset;
			if( ofs_string )
				(*((TEXTCHAR*)ofs_string)) = '+';
#ifdef DEBUG_RESOURCE_NAME_LOOKUP
			lprintf( "Result of %s is %d", name, result );
#endif
			return result;
		}
	}
	return -1; // TXT_STATIC id... invalid to search or locate on...
}

// yes, sometimes we end up having to register psi outside of preload...
// sometimes it works.
static void CPROC InitPSILibrary( void );

//#undef DoRegisterControl
int DoRegisterControlEx( PCONTROL_REGISTRATION pcr, int nSize )
{
	if( pcr )
	{
#ifdef __cplusplus_cli
		// skip these well above legacy controls...
		// if we have 50 controls existing we'd be lucky
		// so creation of 1950 more controls shouldn't happen
		// within the timespan of xperdex.
		static uint32_t ControlID = USER_CONTROL + 2000;
#else
		static uint32_t ControlID = USER_CONTROL;
#endif
		TEXTCHAR namebuf[64], namebuf2[64];
		PCLASSROOT root;
		//lprintf( "Registering control: %s", pcr->name );
		// okay do this so we get our names right?
		InitPSILibrary();
		//pcr->TypeID = ControlID;
		tnprintf( namebuf2, sizeof( namebuf2 ), PSI_ROOT_REGISTRY "/control/%s"
				  , pcr->name );
		root = GetClassRoot( namebuf2 );
		pcr->TypeID = (int)(uintptr_t)GetRegisteredValueExx( root, NULL, "Type", TRUE );
		if( !pcr->TypeID && (StrCaseCmp( pcr->name, "FRAME" )!=0) )
		{
			ControlID = (uint32_t)(uintptr_t)GetRegisteredValueExx( "PSI/Controls", NULL, "User Type ID", TRUE);
			if( !ControlID )
			{
#ifdef __cplusplus_cli
				ControlID = USER_CONTROL + 2000;
#else
				ControlID = USER_CONTROL;
#endif
			}

			pcr->TypeID = ControlID;
			tnprintf( namebuf, sizeof( namebuf ), PSI_ROOT_REGISTRY "/control/%" _32f
					  , ControlID );
			root = RegisterClassAlias( namebuf2, namebuf );
			RegisterValueExx( root, NULL, "Type", FALSE, pcr->name );
			RegisterValueExx( root, NULL, "Type", TRUE, (CTEXTSTR)(uintptr_t)ControlID );

			/*
			tnprintf( namebuf, sizeof( namebuf ), PSI_ROOT_REGISTRY "/control/%" _32f
					  , ControlID );
			root = RegisterClassAlias( namebuf2, namebuf );
			tnprintf( namebuf, sizeof( namebuf ), PSI_ROOT_REGISTRY "/control/%s"
				, pcr->name );
			root = RegisterClassAlias( namebuf2, namebuf );
			*/

			ControlID++; // new control type registered...
			RegisterIntValueEx( (PCLASSROOT)"PSI/Controls", NULL, "User Type ID", (uintptr_t)ControlID );
		}
		else
		{
			//char longbuf[128];
			//tnprintf( longbuf, sizeof( longbuf ), PSI_ROOT_REGISTRY "/control/%s/rtti", pcr->name );
			//if( CheckClassRoot( longbuf ) )
			{
				//lprintf( "Aborting second registration fo same type." );
				//return pcr->TypeID;
			}
		}
#define EXTRA2 stuff.stuff.
#define EXTRA stuff.
		RegisterIntValueEx( root, NULL, "extra", pcr->EXTRA extra );
		RegisterIntValueEx( root, NULL, "width", pcr->EXTRA2 width );
		RegisterIntValueEx( root, NULL, "height", pcr->EXTRA2 height );
		RegisterIntValueEx( root, NULL, "border", pcr->EXTRA default_border );
		// root will now be /psi/controls/(name)/rtti/...=...
		root = GetClassRootEx( root, "rtti" );
		if( pcr->init )
			SimpleRegisterMethod( root, pcr->init
									  , "int", "init", "(PSI_CONTROL,va_list)" );
		if( pcr->load )
			SimpleRegisterMethod( root, pcr->load
									  , "int", "load", "(PSI_CONTROL,PTEXT)" );
		if( pcr->draw )
			SimpleRegisterMethod( root, pcr->draw
									  , "void", "draw", "(PSI_CONTROL)" );
		if( pcr->mouse )
			SimpleRegisterMethod( root, pcr->mouse
									  , "void", "mouse", "(PSI_CONTROL,int32_t,int32_t,uint32_t)" );
		if( pcr->key )
			SimpleRegisterMethod( root, pcr->key
									  , "void", "key", "(PSI_CONTROL,uint32_t)" );
		if( pcr->destroy )
			SimpleRegisterMethod( root, pcr->destroy
									  , "void", "destroy", "(PSI_CONTROL)" );
		if( pcr->save )
			SimpleRegisterMethod( root, pcr->save
									  , "void", "save", "(PSI_CONTROL,PVARTEXT)" );
		if( pcr->prop_page )
			SimpleRegisterMethod( root, pcr->prop_page
									  , "PSI_CONTROL", "get_prop_page", "(PSI_CONTROL)" );
		if( pcr->apply_prop )
			SimpleRegisterMethod( root, pcr->apply_prop
									  , "void", "apply", "(PSI_CONTROL,PSI_CONTROL)" );
		if( pcr->CaptionChanged )
			SimpleRegisterMethod( root, pcr->CaptionChanged
									  , "void", "caption_changed", "(PSI_CONTROL)" );
		if( pcr->FocusChanged )
			SimpleRegisterMethod( root, pcr->FocusChanged
									  , "void", "focus_changed", "(PSI_CONTROL,LOGICAL)" );
		if( pcr->AddedControl )
			SimpleRegisterMethod( root, pcr->AddedControl
									  , "void", "add_control", "(PSI_CONTROL,PSI_CONTROL)" );
		if( nSize > ( offsetof( CONTROL_REGISTRATION, PositionChanging ) + sizeof( pcr->PositionChanging ) ) )
		{
			if( pcr->PositionChanging )
				SimpleRegisterMethod( root, pcr->PositionChanging
										  , "void", "position_changing", "(PSI_CONTROL,LOGICAL)" );
		}
		return ControlID;
	}
	return 0;
}

#ifdef USE_INTERFACES
void GetMyInterface( void )
#define GetMyInterface() if( !g.MyImageInterface || !g.MyDisplayInterface ) GetMyInterface()
{
	if( !g.MyImageInterface )
	{
		g.MyImageInterface = (PIMAGE_INTERFACE)GetInterface( "image" );
		if( !g.MyImageInterface )
			g.MyImageInterface = (PIMAGE_INTERFACE)GetInterface( "real_image" );
		if( !g.MyImageInterface )
		{
#ifndef XBAG
			//if( is_deadstart_complete() )
#endif
			{
#ifndef WIN32
				fprintf( stderr, "Failed to get 'image' interface.  PSI interfaces failing execution." );
#endif
				lprintf( "Failed to get 'image' interface.  PSI interfaces failing execution." );
				lprintf( "and why yes, if we had a display, I suppose we could allow someone to fix this problem in-line..." );
				lprintf( "-------- DIE - DIE - DIE - DIE - DIE - DIE - DIE - DIE - DIE - DIE - DIE ---------" );
				//DebugBreak();
				//exit(0);
			}
			lprintf( "Fail image load once..." );
		}
		SetControlImageInterface( g.MyImageInterface );
	}
	if( !g.MyDisplayInterface )
	{
		g.MyDisplayInterface = (PRENDER_INTERFACE)GetInterface( "render" );
		if( !g.MyImageInterface )
			g.MyDisplayInterface = (PRENDER_INTERFACE)GetInterface( "video" );
		if( !g.MyDisplayInterface )
		{
			{
#ifndef WIN32
				fprintf( stderr, "Failed to get 'render' interface.  PSI interfaces failing execution." );
#endif
				lprintf( "Failed to get 'render' interface.  PSI interfaces failing execution." );
				lprintf( "and why yes, if we had a display, I suppose we could allow someone to fix this problem in-line..." );
				//exit(0);
			}
			lprintf( "Fail render load once..." );
		}
		else
		{
		}
		SetControlInterface( g.MyDisplayInterface );
	}

}
#endif

//--------------------------------------------------------------------------

#ifdef __WATCOM_CPLUSPLUS__
#pragma initialize 47
#endif
PRIORITY_PRELOAD( InitPSILibrary, PSI_PRELOAD_PRIORITY )
{
	//static int bInited;
	if( !GetRegisteredIntValue( PSI_ROOT_REGISTRY "/init", "done" ) )
	//if( !bInited )

	{
		TEXTCHAR namebuf[64], namebuf2[64];

#define REG(name) {   \
			tnprintf( namebuf, sizeof( namebuf ), PSI_ROOT_REGISTRY "/control/%d", name ); \
			tnprintf( namebuf2, sizeof( namebuf2 ), PSI_ROOT_REGISTRY "/control/%s", name##_NAME );             \
			RegisterClassAlias( namebuf2, namebuf );  \
			RegisterValue( namebuf2, "Type", name##_NAME ); \
			RegisterIntValue( namebuf2, "Type", name ); \
		}
#define REG2(name,number) {   \
			tnprintf( namebuf, sizeof( namebuf ), PSI_ROOT_REGISTRY "/control/%d", number ); \
			tnprintf( namebuf2, sizeof( namebuf2 ), PSI_ROOT_REGISTRY "/control/%s", name );             \
			RegisterClassAlias( namebuf2, namebuf );  \
			RegisterValue( namebuf2, "Type", name ); \
			RegisterIntValue( namebuf2, "Type", number ); \
		}
		//lprintf( "Begin registering controls..." );
		REG(CONTROL_FRAME );
		REG(UNDEFINED_CONTROL );
		REG(CONTROL_SUB_FRAME );
		REG(STATIC_TEXT );
		REG(NORMAL_BUTTON );
		REG(CUSTOM_BUTTON );
		REG(IMAGE_BUTTON );
		REG(RADIO_BUTTON );
		REG(EDIT_FIELD );
		REG(SLIDER_CONTROL );
		REG(LISTBOX_CONTROL );
		REG(SCROLLBAR_CONTROL );
		REG(GRIDBOX_CONTROL );
		REG(CONSOLE_CONTROL );
		REG(SHEET_CONTROL );
		REG(COMBOBOX_CONTROL );

		REG2("Color Matrix", BUILTIN_CONTROL_COUNT + 0 );
		REG2("Font Sample", BUILTIN_CONTROL_COUNT + 1 );
		REG2("Font Size Control", BUILTIN_CONTROL_COUNT + 2 );
		REG2("Popup Menu", BUILTIN_CONTROL_COUNT + 3 );
		REG2("Basic Clock Widget", BUILTIN_CONTROL_COUNT + 4 );
		REG2("Scroll Knob", BUILTIN_CONTROL_COUNT + 5 );
		REG2("PSI Console", BUILTIN_CONTROL_COUNT + 6 );
		REG2("Shade Well", BUILTIN_CONTROL_COUNT + 7 );
		REG2("Color Well", BUILTIN_CONTROL_COUNT + 8 );
		{
			int nResources = sizeof( resource_names ) / sizeof( resource_names[0] );
			int n;
			for( n = 0; n < nResources; n++ )
			{
				if( resource_names[n].resource_name_id )
				{
					TEXTCHAR root[256];
					TEXTCHAR old_root[256];
#define TASK_PREFIX "core"
					tnprintf( root, sizeof( root )
							  , PSI_ROOT_REGISTRY "/resources/%s/" TASK_PREFIX "/%s"
							  , resource_names[n].type_name
							  , resource_names[n].resource_name );
					RegisterIntValue( root
										 , "value"
										 , resource_names[n].resource_name_id );
					RegisterIntValue( root
										 , "range"
										 , resource_names[n].resource_name_range );
					tnprintf( root, sizeof( root ), PSI_ROOT_REGISTRY "/resources/%s/" TASK_PREFIX "", resource_names[n].type_name );
					tnprintf( old_root, sizeof( old_root ), PSI_ROOT_REGISTRY "/resources/" TASK_PREFIX "/%s", resource_names[n].type_name );
					RegisterIntValue( root
										 , resource_names[n].resource_name
										 , resource_names[n].resource_name_id );
					RegisterClassAlias( root, old_root );
				}
			}
		}

		RegisterIntValue( PSI_ROOT_REGISTRY "/init", "done", 1 );
	}
}

PFrameBorder PSI_CreateBorder( Image image, int width, int height, int anchors, LOGICAL defines_colors )
{
	GetMyInterface();

 	//if( !g.BorderImage )
	if( image )
	{
		PFrameBorder border;
		INDEX idx;
		LIST_FORALL( g.borders, idx, PFrameBorder, border )
		{
			if( border->BorderImage == image )
				return border;
		}
		border = New( FrameBorder );
		border->hasFill = 0;
		border->drawFill = 0;
		border->defaultcolors = (CDATA*)Allocate( sizeof( DefaultColors ) );
		MemCpy( border->defaultcolors, DefaultColors, sizeof( DefaultColors ) );
		border->BorderImage = image;
		if( border->BorderImage )
		{
			int MiddleSegmentWidth, MiddleSegmentHeight;

			/*
			if( border->BorderImage->width & 1 )
				border->BorderWidth = border->BorderImage->width / 2;
			else
				border->BorderWidth = (border->BorderImage->width-1) / 2;
			if( border->BorderImage->height )
				border->BorderHeight = border->BorderImage->height / 2;
			else
				border->BorderHeight = (border->BorderImage->height-1) / 2;
			*/
			border->BorderWidth = width;
			border->BorderHeight = height;

			border->Border.bAnchorTop = ( ( anchors & BORDER_ANCHOR_TOP_MASK ) >> BORDER_ANCHOR_TOP_SHIFT );
			border->Border.bAnchorLeft = ( ( anchors & BORDER_ANCHOR_LEFT_MASK ) >> BORDER_ANCHOR_LEFT_SHIFT );
			border->Border.bAnchorRight = ( ( anchors & BORDER_ANCHOR_RIGHT_MASK ) >> BORDER_ANCHOR_RIGHT_SHIFT );
			border->Border.bAnchorBottom = ( ( anchors & BORDER_ANCHOR_BOTTOM_MASK ) >> BORDER_ANCHOR_BOTTOM_SHIFT );

			// overcompensate if the settings cause an underflow
			if( border->BorderWidth > border->BorderImage->width )
				border->BorderWidth = border->BorderImage->width / 3;
			if( border->BorderHeight > border->BorderImage->height )
				border->BorderHeight= border->BorderImage->height / 3;
			MiddleSegmentWidth = border->BorderImage->width - (border->BorderWidth*2);
			MiddleSegmentHeight = border->BorderImage->height - (border->BorderHeight*2);

			border->BorderSegment[SEGMENT_TOP_LEFT] = MakeSubImage( border->BorderImage, 0, 0, border->BorderWidth, border->BorderHeight );
			border->BorderSegment[SEGMENT_TOP] = MakeSubImage( border->BorderImage, border->BorderWidth, 0
																	 , MiddleSegmentWidth, border->BorderHeight );
			border->BorderSegment[SEGMENT_TOP_RIGHT] = MakeSubImage( border->BorderImage, border->BorderWidth + MiddleSegmentWidth, 0, border->BorderWidth, border->BorderHeight );
			border->BorderSegment[SEGMENT_LEFT] = MakeSubImage( border->BorderImage
																	  , 0, border->BorderHeight
																	  , border->BorderWidth, MiddleSegmentHeight );
			border->BorderSegment[SEGMENT_CENTER] = MakeSubImage( border->BorderImage
																		 , border->BorderWidth, border->BorderHeight
																		 , MiddleSegmentWidth, MiddleSegmentHeight );
			border->BorderSegment[SEGMENT_RIGHT] = MakeSubImage( border->BorderImage
																		, border->BorderWidth + MiddleSegmentWidth, border->BorderHeight
																		, border->BorderWidth, MiddleSegmentHeight );
			border->BorderSegment[SEGMENT_BOTTOM_LEFT] = MakeSubImage( border->BorderImage
																				, 0, border->BorderHeight + MiddleSegmentHeight
																				, border->BorderWidth, border->BorderHeight );
			border->BorderSegment[SEGMENT_BOTTOM] = MakeSubImage( border->BorderImage
																		 , border->BorderWidth, border->BorderHeight + MiddleSegmentHeight
																	 , MiddleSegmentWidth, border->BorderHeight );
			border->BorderSegment[SEGMENT_BOTTOM_RIGHT] = MakeSubImage( border->BorderImage
																				 , border->BorderWidth + MiddleSegmentWidth, border->BorderHeight + MiddleSegmentHeight
																				 , border->BorderWidth, border->BorderHeight );
			if( 1 /*SACK_GetProfileInt( "SACK/PSI/Frame border"
					, "Use center base colors"
					, 1 )*/ )
			{
				//CDATA *old_colors = border->defaultcolors;
				//border->defaultcolors = (CDATA*)Allocate( sizeof( DefaultColors ) );
				//MemCpy( border->defaultcolors, DefaultColors, sizeof( DefaultColors ) );
				if( border->BorderSegment[SEGMENT_CENTER]->height >= 2
					&& border->BorderSegment[SEGMENT_CENTER]->width >= 7
					&& defines_colors )
				{
#define TestAndSetBaseColor( c, s ) { CDATA src = s; if( src ) border->defaultcolors[c] = src; }
					TestAndSetBaseColor( HIGHLIGHT          , getpixel( border->BorderSegment[SEGMENT_CENTER], 0, 0 ) );
					TestAndSetBaseColor( SHADE               , getpixel( border->BorderSegment[SEGMENT_CENTER], 0, 1 ) );
					TestAndSetBaseColor( NORMAL              , getpixel( border->BorderSegment[SEGMENT_CENTER], 1, 0 ) );
					TestAndSetBaseColor( SHADOW              , getpixel( border->BorderSegment[SEGMENT_CENTER], 1, 1 ) );
					TestAndSetBaseColor( TEXTCOLOR           , getpixel( border->BorderSegment[SEGMENT_CENTER], 2, 0 ) );
					TestAndSetBaseColor( CAPTIONTEXTCOLOR    , getpixel( border->BorderSegment[SEGMENT_CENTER], 3, 0 ) );
					TestAndSetBaseColor( CAPTION             , getpixel( border->BorderSegment[SEGMENT_CENTER], 3, 1 ) );
					TestAndSetBaseColor( INACTIVECAPTIONTEXTCOLOR, getpixel( border->BorderSegment[SEGMENT_CENTER], 4, 0 ) );
					TestAndSetBaseColor( INACTIVECAPTION     , getpixel( border->BorderSegment[SEGMENT_CENTER], 4, 1 ) );
					TestAndSetBaseColor( SELECT_TEXT         , getpixel( border->BorderSegment[SEGMENT_CENTER], 5, 0 ) );
					TestAndSetBaseColor( SELECT_BACK         , getpixel( border->BorderSegment[SEGMENT_CENTER], 5, 1 ) );
					TestAndSetBaseColor( EDIT_TEXT           , getpixel( border->BorderSegment[SEGMENT_CENTER], 6, 0 ) );
					TestAndSetBaseColor( EDIT_BACKGROUND     , getpixel( border->BorderSegment[SEGMENT_CENTER], 6, 1 ) );
					TestAndSetBaseColor( SCROLLBAR_BACK      , getpixel( border->BorderSegment[SEGMENT_CENTER], 7, 0 ) );
				}
				else
					TestAndSetBaseColor( NORMAL              , 0 );

				border->BorderSegment[SEGMENT_CENTER] = MakeSubImage( border->BorderImage
					, border->BorderWidth, border->BorderHeight
					, MiddleSegmentWidth, MiddleSegmentHeight );

				if( MiddleSegmentWidth > 2 * border->BorderWidth )
					border->hasFill = TRUE;
			}
		}
		AddLink( &g.borders, border );
		return border;
	}
	return NULL;
}

void PSI_SetFrameBorder( PSI_CONTROL pc, PFrameBorder border )
{
	pc->border = border;
	if( !( pc->BorderType & BORDER_USER_PROC ) )
		pc->DrawBorder = (pc->border&&pc->border->BorderImage)?DrawFancyFrame:DrawNormalFrame;
	if( border )
		border->drawFill = 1;
	UpdateSurface( pc );
}

//#define basecolor(pc) ((pc)?((pc)->border?(pc)->border->defaultcolors:(pc)->basecolors):(g.DefaultBorder?g.DefaultBorder->defaultcolors:DefaultColors))
CDATA *basecolor( PSI_CONTROL pc )
{
	//xlprintf( "get base color pc: %p %p %p", pc, pc?pc->basecolors:0, pc?pc->border:0 );
	if( pc )
		if( ( pc )->border ) {
			return ( pc )->border->defaultcolors;
		} else {
			if( pc->basecolors )
				return ( pc )->basecolors;
			return basecolor( pc->parent );
		}
	else
		if (g.DefaultBorder)
			return g.DefaultBorder->defaultcolors;
		else
			return DefaultColors;
}

void TryLoadingFrameImage( void )
{
	if( g.flags.system_color_set )
		return;
#ifndef __NO_OPTIONS__
	g.StopButtonPad = SACK_GetProfileInt( "SACK/PSI"
		, "Frame close button pad"
		, 2 );
#else
	g.StopButtonPad = 2;
#endif
	if( !g.StopButton )
	{
		TEXTCHAR buffer[256];
#ifndef __NO_OPTIONS__
		SACK_GetProfileStringEx( GetProgramName(), "SACK/PSI/Frame close button image", "stop_button.png", buffer, sizeof( buffer ), TRUE );
#else
		StrCpy( buffer, "stop_button.png" );
#endif
		g.StopButton = LoadImageFileFromGroup( GetFileGroup( "Images", "./images" ), buffer );
	}
	if( !g.StopButtonPressed )
	{
		TEXTCHAR buffer[256];
#ifndef __NO_OPTIONS__
		SACK_GetProfileStringEx( GetProgramName(), "SACK/PSI/Frame close button pressed image", "stop_button_pressed.png", buffer, sizeof( buffer ), TRUE );
#else
		StrCpy( buffer, "stop_button_pressed.png" );
#endif
		g.StopButtonPressed = LoadImageFileFromGroup( GetFileGroup( "Images", "./images" ), buffer );
	}
 	if( !g.FrameCaptionImage )
	{
		TEXTCHAR buffer[256];
#ifndef __NO_OPTIONS__
		SACK_GetProfileStringEx( GetProgramName(), "SACK/PSI/Frame caption background", "", buffer, sizeof( buffer ), TRUE );
#else
		StrCpy( buffer, "" );
#endif
		if( buffer[0] )
			g.FrameCaptionImage = LoadImageFileFromGroup( GetFileGroup( "Images", "./images" ), buffer );
	}
	if( !g.FrameCaptionFocusedImage )
	{
		TEXTCHAR buffer[256];
#ifndef __NO_OPTIONS__
		SACK_GetProfileStringEx( GetProgramName(), "SACK/PSI/Frame caption focused background", "", buffer, sizeof( buffer ), TRUE );
#else
		StrCpy( buffer, "" );
#endif
		if( buffer[0] )
			g.FrameCaptionFocusedImage = LoadImageFileFromGroup( GetFileGroup( "Images", "./images" ), buffer );
	}
 	if( !g.DefaultBorder )
	{
		TEXTCHAR buffer[256];
		Image border_image;
		int width, height;
		int anchors;
#ifndef __NO_OPTIONS__
		SACK_GetProfileStringEx( GetProgramName(), "SACK/PSI/Frame border image", "frame2.png", buffer, sizeof( buffer ), TRUE );
#else
		StrCpy( buffer, "frame_border.png" );
#endif
		border_image = LoadImageFileFromGroup( GetFileGroup( "Images", "./images" ), buffer );
		if( border_image )
		{
#ifndef __NO_OPTIONS__
			width = SACK_GetProfileIntEx( GetProgramName()
					, "SACK/PSI/Frame border/Width"
					, 0, TRUE );
			height = SACK_GetProfileIntEx( GetProgramName()
					, "SACK/PSI/Frame border/Height"
					, 0, TRUE );
			anchors = 0;
			anchors |= SACK_GetProfileIntEx( GetProgramName()
					, "SACK/PSI/Frame border/Anchor Top"
					, 0, TRUE )<<BORDER_ANCHOR_TOP_SHIFT;
			anchors |= SACK_GetProfileIntEx( GetProgramName()
					, "SACK/PSI/Frame border/Anchor Bottom"
					, 0, TRUE )<<BORDER_ANCHOR_BOTTOM_SHIFT;
			anchors |= SACK_GetProfileIntEx( GetProgramName()
					, "SACK/PSI/Frame border/Anchor Left"
					, 0, TRUE )<<BORDER_ANCHOR_LEFT_SHIFT;
			anchors |= SACK_GetProfileIntEx( GetProgramName()
					, "SACK/PSI/Frame border/Anchor Right"
					, 0, TRUE )<<BORDER_ANCHOR_RIGHT_SHIFT;

			// overcompensate if the settings cause an underflow
			if( !width || (2*width) >= border_image->width )
				width = border_image->width / 3;
			if( !height || (2*height) >= border_image->height )
				height = border_image->height / 3;
#else
			if( border_image->width & 1 )
				width = border_image->width / 2;
			else
				width = (border_image->width-1) / 2;
			if( border_image->height )
				height = border_image->height / 2;
			else
				height = (border_image->height-1) / 2;
#endif
			g.DefaultBorder = PSI_CreateBorder( border_image, width, height, anchors
					, SACK_GetProfileInt( "SACK/psi/Frame Border", "Has Control theme colors", 0 ) );
		}
	}
}

#ifdef __cplusplus
static void OnDisplayConnect( "@00 PSI++ Core" )( struct display_app*app, struct display_app_local ***pppLocal )
#else
static void OnDisplayConnect( "@00 PSI Core" )(struct display_app*app, struct display_app_local ***pppLocal)
#endif
{
	PFrameBorder border;
	INDEX idx;
	GetMyInterface();
	ReuseImage( g.StopButton );
	ReuseImage( g.StopButtonPressed );
	LIST_FORALL( g.borders, idx, PFrameBorder, border )
	{
		{
			int n;
			ReuseImage( border->BorderImage );
			for( n = 0; n < 9; n++ )
			{
				ReuseImage( border->BorderSegment[n] );
			}
		}
	}
}

// this can be run very late...
PRELOAD( DefaultControlStartup )
{
#ifndef __NO_OPTIONS__
	g.flags.bLogDebugUpdate = SACK_GetProfileIntEx( GetProgramName(), "SACK/PSI/Log Control Updates", 0, TRUE );
	g.flags.bLogDetailedMouse = SACK_GetProfileIntEx( GetProgramName(), "SACK/PSI/Log Mouse Events", 0, TRUE );
	g.flags.bLogKeyEvents = SACK_GetProfileIntEx( GetProgramName(), "SACK/PSI/Log Key Events", 0, TRUE );
	g.flags.bLogSuperDetailedMouse = SACK_GetProfileIntEx( GetProgramName(), "SACK/PSI/Log Mouse Events extra detail", 0, TRUE );
#endif
}

PSI_PROC( PIMAGE_INTERFACE, SetControlImageInterface )( PIMAGE_INTERFACE DisplayInterface )
{
	g.MyImageInterface = DisplayInterface;

	if( !DisplayInterface ) return DisplayInterface;

	if( !DefaultColors[0] ) {
		DefaultColors[0] = Color( 192, 192, 192 ); // highlight
		DefaultColors[1] = AColor( 53, 96, 89, 225 ); // normal
		DefaultColors[2] = Color( 35, 63, 57 ); // shade
		DefaultColors[3] = Color( 0, 0, 1 ); // shadow
		DefaultColors[4] = AColor( 0, 240, 240, 255 ); // text
		DefaultColors[5] = Color( 88, 124, 200 ); // caption background
		DefaultColors[6] = Color( 240, 240, 240 ); // cannot be black(0). caption text
		DefaultColors[7] = Color( 89, 120, 120 ); // inactive caption background
		DefaultColors[8] = Color( 0, 0, 1 );     // inactive caption text (not black)
		DefaultColors[9] = Color( 0, 0, 128 );  // text select background
		DefaultColors[10] = Color( 220, 220, 255 ); // text select foreground
		DefaultColors[11] = AColor( 192, 192, 192, 225 ); // edit background
		DefaultColors[12] = Color( 0, 0, 1 );  // edit text
		DefaultColors[13] = Color( 120, 120, 180 ); // scroll bar...
	}

#ifdef __ANDROID__
	if( !g.default_font ) {
		uint32_t w, h;
		GetDisplaySize( &w, &h );
		if( h > w )
			g.default_font = RenderFontFileScaledEx( "%resources%/fonts/MyriadPro.ttf", w / 34, h / 48, NULL, NULL, 2/*FONT_FLAG_8BIT*/, NULL, NULL );
		else
			g.default_font = RenderFontFileScaledEx( "%resources%/fonts/MyriadPro.ttf", w / 58, h / 32, NULL, NULL, 2/*FONT_FLAG_8BIT*/, NULL, NULL );
	}
#else
	if( !g.default_font ) {
		TEXTCHAR buffer[256];
		CTEXTSTR default_name;

		uint32_t w, h;
		int bias_x, bias_y;
		GetFileGroup( "Resources", "@/../Resources" );
		//GetDisplaySize( &w, &h );
		//g.default_font = RenderFontFileScaledEx( "%resources%/fonts/rod.ttf", 20, 20, NULL, NULL, 0*2/*FONT_FLAG_8BIT*/, NULL, NULL );
		//g.default_font = RenderFontFileScaledEx( "rod.ttf", 18, 18, NULL, NULL, 2/*FONT_FLAG_8BIT*/, NULL, NULL );
		if( sack_exists( "c:/windows/fonts/msyh.ttf" ) )
			default_name = "msyh.ttf";
		else if( sack_exists( "c:/windows/fonts/msyh.ttc" ) )
			default_name = "msyh.ttc";
		else
			default_name = "arialbd.ttf";
		SACK_GetProfileString( "SACK/PSI/Font", "Default File", default_name, buffer, 256 );
		w = SACK_GetProfileInt( "SACK/PSI/Font", "Default Width", 18 );
		h = SACK_GetProfileInt( "SACK/PSI/Font", "Default Height", 18 );
		g.default_font = RenderFontFileScaledEx( buffer, w, h, NULL, NULL, 2/*FONT_FLAG_8BIT*/, NULL, NULL );
		bias_x = SACK_GetProfileInt( "SACK/PSI/Font", "Bias X", 0 );
		bias_y = SACK_GetProfileInt( "SACK/PSI/Font", "Bias Y", 0 );
		//lprintf( "default font %p %d,%d", g.default_font, bias_x, bias_y );
		//SetFontBias( g.default_font, bias_x, bias_y );
	}
#endif

#ifndef PSI_SERVICE
#  ifdef USE_INTERFACES
	return g.MyImageInterface = DisplayInterface;
#endif
#endif
	return DisplayInterface;
}


PSI_PROC( PRENDER_INTERFACE, SetControlInterface )( PRENDER_INTERFACE DisplayInterface )
{
	g.MyDisplayInterface = DisplayInterface;
	if( !( g.flags.always_draw = RequiresDrawAll() ) )
		g.flags.allow_threaded_draw = AllowsAnyThreadToUpdate();
	g.flags.allow_copy_from_render = VidlibRenderAllowsCopy();

	return DisplayInterface;
}
//---------------------------------------------------------------------------
// basic controls implement begin here!


//---------------------------------------------------------------------------

PSI_PROC( void, AlignBaseToWindows )( void )
{
#ifdef _WIN32
	int sys_r;
	int sys_g;
	int sys_b;
	int sys_a;
	int tmp;

	if( !g.MyImageInterface )
		GetMyInterface();
#define Swap(i)    ( (tmp = i),( sys_r = ((tmp) & 0xFF)), (sys_g = ((tmp>>8) & 0xFF)),(sys_b = ((tmp >>16) & 0xFF)),(sys_a = 0xFF),AColor(sys_r,sys_g,sys_b,sys_a) )
	g.flags.system_color_set = 1;
	 DefaultColors[HIGHLIGHT        ] =  Swap(GetSysColor( COLOR_3DHIGHLIGHT));
	//if( !g.BorderImage )
	DefaultColors[NORMAL           ] =  Swap(GetSysColor(COLOR_3DFACE ));
	DefaultColors[SHADE            ] =  Swap(GetSysColor(COLOR_3DSHADOW ));
	DefaultColors[SHADOW           ] =  Swap(GetSysColor(COLOR_3DDKSHADOW ));
	DefaultColors[TEXTCOLOR        ] =  Swap(GetSysColor(COLOR_BTNTEXT ));
	DefaultColors[CAPTION          ] =  Swap(GetSysColor(COLOR_ACTIVECAPTION ));
	DefaultColors[CAPTIONTEXTCOLOR] =  Swap(GetSysColor( COLOR_CAPTIONTEXT));
	DefaultColors[INACTIVECAPTION ] =  Swap(GetSysColor(COLOR_INACTIVECAPTION ));
	DefaultColors[INACTIVECAPTIONTEXTCOLOR]=Swap(GetSysColor(COLOR_INACTIVECAPTIONTEXT ));
	DefaultColors[SELECT_BACK      ] =  Swap(GetSysColor(COLOR_HIGHLIGHT ));
	DefaultColors[SELECT_TEXT      ] =  Swap(GetSysColor(COLOR_HIGHLIGHTTEXT ));
	DefaultColors[EDIT_BACKGROUND ] =  Swap(GetSysColor(COLOR_WINDOW ));
	DefaultColors[EDIT_TEXT       ] =  Swap(GetSysColor(COLOR_WINDOWTEXT ));
	DefaultColors[SCROLLBAR_BACK  ] =  Swap(GetSysColor(COLOR_SCROLLBAR ));
#endif
    // No base to set to - KDE/Gnome/E/?
}

//---------------------------------------------------------------------------

PSI_PROC( void, SetBaseColor )( INDEX idx, CDATA c )
{
	//lprintf( "Color %d was %08X and is now %08X", idx, defaultcolor[idx], c );
	DefaultColors[idx] = c;
}

PSI_PROC( CDATA, GetBaseColor )( INDEX idx )
{
	return DefaultColors[idx] ;
}

//---------------------------------------------------------------------------

PSI_PROC( void, SetControlColor )( PSI_CONTROL pc, INDEX idx, CDATA c )
{
	if( pc )
	{
		if( !pc->basecolors || basecolor( pc ) == DefaultColors ) {
			if( !pc->border ) {
				pc->basecolors = NewArray( CDATA, sizeof( DefaultColors ) / sizeof( CDATA ) );
				MemCpy( pc->basecolors, DefaultColors, sizeof( DefaultColors ) );
			} // otherwise we'll be setting the border default... which can be shared...
		}
		basecolor( pc )[idx] = c;
	}
}

PSI_PROC( CDATA, GetControlColor )( PSI_CONTROL pc, INDEX idx )
{
	return basecolor(pc)[idx];
}

//---------------------------------------------------------------------------

// dir 0 here only... in case we removed ourself from focusable
// dir -1 go backwards
// dir 1 go forwards
#define FFF_HERE      0
#define FFF_FORWARD   1
#define FFF_BACKWARD -1
void FixFrameFocus( PPHYSICAL_DEVICE pf, int dir )
{
	if( pf )
	{
		PSI_CONTROL pc = pf->common;
#ifdef DEBUG_FOCUS_STUFF
		lprintf( "FixFrameFocus...." );
#endif
		if( !pc->flags.bDestroy  )
		{
			PSI_CONTROL pcCurrent, pcStart;
			int bLooped = FALSE, bTryAgain = FALSE;
			pcStart = pcCurrent = pf->pFocus;
			if( !pcCurrent )
			{
				// have to focus SOMETHING
				pcCurrent = pf->pFocus = pc->child;
				//pcCurrent = pcStart = pc->child;
				//return; // doesn't matter where the focus in the frame is.
			}
			// no child controls to focus...
			if( !pcCurrent )
				return;
			if( dir == FFF_FORWARD )
				pcCurrent = pcCurrent->next;
			else if( dir == FFF_BACKWARD )
				pcCurrent = pcCurrent->prior;
			do
			{
				if( bTryAgain )
					bLooped = TRUE;
				bTryAgain = FALSE;
				while( pcCurrent )
				{
					if( (!pcCurrent->flags.bHidden) &&(!pcCurrent->flags.bNoFocus) &&
						((!pcCurrent->flags.bDisable) ||
						 (pcCurrent == pcStart )))
					{
						if( pcStart && pcCurrent != pcStart )
							SetCommonFocus( pcCurrent );
						break;
					}
					if( ( dir == FFF_FORWARD ) || ( dir == FFF_HERE ) )
						pcCurrent = pcCurrent->next;
					else
						pcCurrent = pcCurrent->prior;
				}
				if( !pcCurrent && ( pcCurrent = pc->child ) )
				{
					if( dir != FFF_FORWARD )
					{
						// go to last.
						while( pcCurrent->next )
							pcCurrent = pcCurrent->next;
					}
					bTryAgain = TRUE;
				}
			}while( bTryAgain && !bLooped );
		}
	}
}

//---------------------------------------------------------------------------

void RestoreBackground( PSI_CONTROL pc, P_IMAGE_RECTANGLE r )
{
	PSI_CONTROL parent;
	if( pc )
	{
		for( parent = pc->parent; parent; parent = parent->parent )
		{
			if( parent->flags.bDirty )
			{
				break;
			}
		}
		if( !parent )
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "Restoring orignal background... " );
#endif
			pc->flags.bParentCleaned = 1;
			BlotImageSizedTo( pc->Surface, pc->OriginalSurface,  r->x, r->y, r->x, r->y, r->width, r->height );
		}
		else
		{
			lprintf( "parent would have to draw before I can restore my control's background" );
		}
	}

}

//---------------------------------------------------------------------------

void InvokeControlHidden( PSI_CONTROL pc )
{
	void (CPROC *OnHidden)(PSI_CONTROL);
	PCLASSROOT data = NULL;
	PCLASSROOT event_root = GetClassRootEx( pc->class_root, "hide_control" );
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( event_root, &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		OnHidden = GetRegisteredProcedureExx( data,(CTEXTSTR)NULL,void,name,(PSI_CONTROL));
		if( OnHidden )
		{
			OnHidden( pc );
		}
	}
}

//---------------------------------------------------------------------------

void InvokeControlRevealed( PSI_CONTROL pc )
{
	void (CPROC *OnReveal)(PSI_CONTROL);
	PCLASSROOT data = NULL;
	PCLASSROOT event_root = GetClassRootEx( pc->class_root, "reveal_control" );
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( event_root, &data );
		 name;
		  name = GetNextRegisteredName( &data ) )
	{
		OnReveal = GetRegisteredProcedureExx( data,(CTEXTSTR)NULL,void,name,(PSI_CONTROL));
		if( OnReveal )
		{
			OnReveal( pc );
		}
	}
}

//---------------------------------------------------------------------------

// this always works from the root dialog
// ... which causes in essense the whole window
// to update ... we hate this... and really only have to go
// as far as the last non-transparent image...
void UpdateSomeControls( PSI_CONTROL pc, P_IMAGE_RECTANGLE pRect )
{
	if( !g.flags.always_draw )
	{
	PPHYSICAL_DEVICE pf = GetFrame( pc )->device;
	//IMAGE_RECTANGLE _rect;
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		lprintf( "Update Some Controls - (all controls within rect %d) ", pc->flags.bInitial );
#endif
	//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, GetFrame( pc ) );
	//cpg26dec2006 c:\work\sack\src\psilib\controls.c(741): Warning! W202: Symbol 'prior_flag' has been defined, but not referenced
	//cpg26dec2006 	int prior_flag;
	// include bias to surface - allow everyone
	// else to think in area within frame surface...

	// add the final frame surface offset...
	// this should remove the need for edit_bias to be added...
	// plus - now pc will refer to the frame, and is where
	// we desire to be drawing anyhow....
	// working down within the frame/controls ....
	// but so far - usage has been from the control's rect,
	// and not its surface, therefore subracting it's surface rect was
	// wrong - but this allows us to cleanly subract the last, and not
	// the first...
	if( !pRect )
		return;
	if( !pc )
		return;
	if( pc->flags.bHidden )
	{
		lprintf( "Control is hidden, skipping it." );
		return;
		//continue;
	}
	//lprintf( "UpdateSomeControls - input rect is %d,%d  %d,%d", pRect->x, pRect->y, pRect->width, pRect->height );
	//lprintf( "UpdateSomeControls - changed rect is %d,%d  %d,%d", pRect->x, pRect->y, pRect->width, pRect->height );

	// Uhmm well ... transporting dirty_rect ... on the control
	// passing a rect in...
	//(*pRect) = pc->dirty_rect;

	if( pf && !pc->flags.bInitial && pf->pActImg )
	{
		IMAGE_RECTANGLE clip;
		IMAGE_RECTANGLE surf_rect;
		clip.x = 0;
		clip.y = 0;
		clip.width = pc->surface_rect.width;
		clip.height = pc->surface_rect.width;
		if( IntersectRectangle( &surf_rect, pRect, &clip ) )
		{
			surf_rect.x += pc->surface_rect.x;
			surf_rect.y += pc->surface_rect.y;
			while( pc && pc->parent && !pc->device )
			{
				// don't subract the first surface
				// but do subtract the last surface...
				surf_rect.x += pc->rect.x;
				surf_rect.y += pc->rect.y;
				pc = pc->parent;;
				surf_rect.x += pc->surface_rect.x;
				surf_rect.y += pc->surface_rect.y;
			}
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "Some controls using normal updatecommon to draw..." );
#endif
			// enabled minimal update region...
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
			{
				lprintf( "Blatting color to surface so that we have something update?!" );
				lprintf( "Update portion %d,%d to %d,%d", surf_rect.x, surf_rect.y, surf_rect.width, surf_rect.height );
			}
#endif
#ifdef BLAT_COLOR_UPDATE_PORTION
			BlatColorAlpha( pc->Window,  surf_rect.x
							  ,  surf_rect.y, surf_rect.width, surf_rect.height, SetAlpha( BASE_COLOR_PURPLE, 0x20 ) );
#endif
			UpdateDisplayPortion( pf->pActImg
									  , surf_rect.x
									  , surf_rect.y
									  , surf_rect.width
									  , surf_rect.height );
		}
	}
	}
}

//---------------------------------------------------------------------------

void SmudgeSomeControlsWork( PSI_CONTROL pc, P_IMAGE_RECTANGLE pRect )
{
	IMAGE_RECTANGLE wind_rect;
	IMAGE_RECTANGLE surf_rect;

	for( ;pc; pc = pc->next )
	{
		{
			PSI_CONTROL parent;
			for( parent = pc; parent; parent = parent->parent )
			{
				if( parent->flags.bNoUpdate || parent->flags.bHidden )
				{
					lprintf( "a control %p (self, or some parent %p) has %s or %s"
							  , pc, parent
							  , parent->flags.bNoUpdate?"noupdate":"..."
							  , parent->flags.bHidden?"hidden":"..."
							  );
					break;
				}
			}
			if( parent )
			{
				lprintf( "ABORTING SMUDGE" );
				continue;
			}
		}
		if( pc->flags.bHidden || pc->flags.bNoUpdate )
		{
			lprintf( "Control is hidden, skipping it." );
			continue;
		}
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			lprintf( "updating some controls... rectangles and stuff." );
#endif
	 //Log( "Update some controls...." );
		if( !IntersectRectangle( &wind_rect, pRect, &pc->rect ) )
			continue;
		wind_rect.x -= pc->rect.x;
		wind_rect.y -= pc->rect.y;
		 // bound window rect (frame update)
			// The update region may be
		if( IntersectRectangle( &surf_rect, &wind_rect, &pc->surface_rect ) )
		{
			surf_rect.x -= pc->surface_rect.x;
			surf_rect.y -= pc->surface_rect.y;
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( "Some controls using normal updatecommon to draw..." );
#endif
			// enabled minimal update region...
			pc->dirty_rect = surf_rect;
			SmudgeCommon( pc ); // and all children, if dirtied...
		}
		else
		{
			// wind_rect is the merge of the update needed
			// and the window's bounds, but none of the surface
			// setting the image bound to this will short many things like blotting the
			//fancy image borders.
			// yes redundant with above, but need to fix the image pos
			// AFTER the update... and well....
			//Log( "Hit the rectange, but didn't hit the content... so update border only." );
			if( pc->DrawBorder )
			{
#ifdef DEBUG_BORDER_DRAWING
				lprintf( "Drawing border ..." );
#endif
				pc->DrawBorder( pc );
			}
			if( pc->device )
			{
				//void DrawFrameCaption( PSI_CONTROL );
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( "Drew border, drawing caption uhmm update some work controls" );
#endif
				DrawFrameCaption( pc );
			}
		}
	}
}

//---------------------------------------------------------------------------

// this always works from the root dialog
// ... which causes in essense the whole window
// to update ... we hate this... and really only have to go
// as far as the last non-transparent image...
void SmudgeSomeControls( PSI_CONTROL pc, P_IMAGE_RECTANGLE pRect )
{
	PPHYSICAL_DEVICE pf = GetFrame( pc )->device;
	//IMAGE_RECTANGLE _rect;
	//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, GetFrame( pc ) );
	int prior_flag;
	// include bias to surface - allow everyone
	// else to think in area within frame surface...

	// add the final frame surface offset...
	// this should remove the need for edit_bias to be added...
	// plus - now pc will refer to the frame, and is where
	// we desire to be drawing anyhow....
	// working down within the frame/controls ....
	// but so far - usage has been from the control's rect,
	// and not its surface, therefore subracting it's surface rect was
	// wrong - but this allows us to cleanly subract the last, and not
	// the first...
	if( !pRect )
		return;
	//lprintf( "SmudgeSomeControls - input rect is %d,%d  %d,%d", pRect->x, pRect->y, pRect->width, pRect->height );
	while( pc && pc->parent && !pc->device )
	{
		// don't subract the first surface
		// but do subtract the last surface...
		pRect->x += pc->rect.x;
		pRect->y += pc->rect.y;
		pc = pc->parent;;
		pRect->x += pc->surface_rect.x;
		pRect->y += pc->surface_rect.y;
	}
	//lprintf( "SmudgeSomeControls - changed rect is %d,%d  %d,%d", pRect->x, pRect->y, pRect->width, pRect->height );
	prior_flag = pc->flags.bInitial;
	pc->flags.bInitial = 1;
	SmudgeSomeControlsWork( pc, pRect );
	pc->flags.bInitial = prior_flag;
	// Uhmm well ... transporting dirty_rect ... on the control
	// passing a rect in...
	(*pRect) = pc->dirty_rect;

	if( pf && !pc->flags.bInitial && pf->pActImg )
	{
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
		{
			lprintf( "Blatting color to surface so that we have something update?!" );
			lprintf( "Update portion %d,%d to %d,%d", pRect->x, pRect->y, pRect->width, pRect->height );
		}
#endif
		/*
		 UpdateDisplayPortion( pf->pActImg
									, pRect->x
									, pRect->y
									, pRect->width
									, pRect->height );
		*/
	 }
}

//---------------------------------------------------------------------------

static int OnDrawCommon( "Frame" )( PSI_CONTROL pc )
{
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		lprintf( "-=-=-=-=- Output Frame background..." );
#endif
	if( !pc->border || !pc->border->hasFill ) {
		if( !pc->parent )
			BlatColor( pc->Surface, 0, 0, pc->surface_rect.width, pc->surface_rect.height, basecolor( pc )[NORMAL] );
		else
			BlatColorAlpha( pc->Surface, 0, 0, pc->surface_rect.width, pc->surface_rect.height, basecolor( pc )[NORMAL] );
	}
	else {
		PFrameBorder border = pc->border;
		if( (border->BorderSegment[SEGMENT_CENTER]->width > border->BorderWidth * 3) && (!pc->flags.bInitial || border->drawFill) ) {
			BlotScaledImageSizedEx( pc->Window, border->BorderSegment[SEGMENT_CENTER]
				, pc->surface_rect.x, pc->surface_rect.y
				, pc->surface_rect.width, pc->surface_rect.height
				, 0, 0
				, border->BorderSegment[SEGMENT_CENTER]->width, border->BorderSegment[SEGMENT_CENTER]->height
				, ALPHA_TRANSPARENT, BLOT_COPY );
			border->drawFill = 0;
		}
	}
	DrawFrameCaption( pc );
	return 1;
}

//--------------------------------------------------------------------------

static void OnDrawCommonDecorations( "Frame" )( PSI_CONTROL pc, PSI_CONTROL child )
{
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		lprintf( "-=-=-=-=- Output Frame decorations... %p  %p", pc, child );
#endif
	if( pc->device )
		DrawHotSpots( pc, &pc->device->EditState, child );
}

//--------------------------------------------------------------------------
// forward declaration cause we're lazy and don't want to re-wind the below routines...
typedef struct psi_penging_rectangle_tag
{
	struct {
		BIT_FIELD bHasContent : 1;
		BIT_FIELD bTmpRect : 1;
		BIT_FIELD bInitialized : 1;
	} flags;
   // this shouldn't be a thread-shared structure.
	//CRITICALSECTION cs;
	int32_t x, y;
   uint32_t width, height;
} PSI_PENDING_RECT, *PPSI_PENDING_RECT;
static void DoUpdateCommonEx( PPSI_PENDING_RECT upd, PSI_CONTROL pc, int bDraw, int level DBG_PASS );

static void DoUpdateFrame( PSI_CONTROL pc
								 , int x, int y
								 , int w, int h
								 , int surface_bias
								  DBG_PASS)
{
	static int level;
	PPHYSICAL_DEVICE pf = NULL;

	if( pc )
	{
		if( pc->flags.bHidden )
			return;
		pf = pc->device;
	}
	else
	{
		lprintf( "Why did ypu pass a NULL control to this?! ( the event to close happeend before updat" );
		return;
	}
#if DEBUG_UPDAATE_DRAW > 2
	if( g.flags.bLogDebugUpdate )
		_lprintf(DBG_RELAY)( "Do Update frame.. x, y on frame of %d,%d,%d,%d ", x, y, w, h );
#endif
	level++;
	if( pc && !pf ) // might just not have a device but be a root?
	{
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			lprintf( "Stepping to parent, adding my surface rect and my rect to the coordinate updated... %d+%d+%d %d+%d+%d"
					 , x, pc->parent->rect.x, pc->parent->surface_rect.x
					 , y, pc->parent->rect.y, pc->parent->surface_rect.y
					 );
#endif
		if( pc->parent && !pc->device ) // otherwise we're probably still creating the thing, and it's in bInitial?
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "stepping to parent, assuming I'm copying my surface, so update appropriately" );
#endif
#ifdef BLAT_COLOR_UPDATE_PORTION
			TESTCOLOR=SetAlpha( BASE_COLOR_BLUE, 128 );
#endif
			DoUpdateFrame( pc->parent
							 , (pc->parent->device?0:pc->parent->rect.x) + pc->parent->surface_rect.x + x
							 , (pc->parent->device?0:pc->parent->rect.y) + pc->parent->surface_rect.y + y
							 , w, h
							 , FALSE
							  DBG_RELAY
							 );
		}
	}
	else if( pf )
	{
		if( pc->flags.bInitial || pc->flags.bNoUpdate )
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "Failing to update to screen cause %s and/or %s", pc->flags.bInitial?"it's initial":""
						 , pc->flags.bNoUpdate ?"it's no update...":"...");
#endif
			level--;
			return;
		}

		{
			int bias_x = 0;
			int bias_y = 0;
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				_xlprintf( 1 DBG_RELAY )( "updating display portion %d,%d (%d,%d)"
												, bias_x + x
												, bias_y + y
												, w, h );
#endif
#ifdef BLAT_COLOR_UPDATE_PORTION
			BlatColorAlpha( pc->Window
							  , x //+ pc->surface_rect.x
							  , y //+ pc->surface_rect.y
							  , w, h, TESTCOLOR );
#endif
			UpdateDisplayPortion( pf->pActImg
									  , bias_x + x
									  , bias_y + y
									  , w, h );
		}
	}
	level--;
}

//---------------------------------------------------------------------------
PSI_PROC( void, UpdateFrameEx )( PSI_CONTROL pc
									  , int x, int y
									  , int w, int h DBG_PASS)
{
	PPHYSICAL_DEVICE pf = pc->device;
   //ValidatedControlData( PFRAME, CONTROL_FRAME, pf, pc );
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		_lprintf(DBG_RELAY)( "Update Frame (Another flavor ) %p %p  %d,%d %d,%d", pc, pf, x, y, w, h );
#endif
   //_xlprintf( 1 DBG_RELAY )( "Update Frame ------------" );
	if( pc && !pf )
	{
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
		{
			//lprintf( "Stepping to parent, adding my surface rect and my rect to the coordinate updated..." );
			lprintf( "stepping to parent, assuming I'm copying my surface, so update appropriately" );
			lprintf( "Stepping to parent, adding my surface rect and my rect to the coordinate updated... %d+%d+%d %d+%d+%d"
					 , x, pc->parent->rect.x, pc->parent->surface_rect.x
					 , y, pc->parent->rect.y, pc->parent->surface_rect.y
					 );
		}
#endif
		if( pc->parent )
			UpdateFrameEx( pc->parent
							 , pc->rect.x + pc->surface_rect.x + x
							 , pc->rect.y + pc->surface_rect.y + y
							 , w?w:pc->rect.width, h?h:pc->rect.height DBG_RELAY );
	}
	else if( pf )
	{
		if( pc->flags.bInitial || pc->flags.bNoUpdate )
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "Faileing to update to screen cause we're initial or it's no update..." );
#endif
			return;
		}

#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			_xlprintf( 1 DBG_RELAY )( "updating display portion %d,%d (%d,%d)"
											, pc->surface_rect.x + x
											, pc->surface_rect.y + y
											, w, h );
#endif
#ifdef BLAT_COLOR_UPDATE_PORTION
		BlatColorAlpha( pc->Window,  x + pc->surface_rect.x
						  ,  y + pc->surface_rect.y
						  , w, h, SetAlpha( BASE_COLOR_PURPLE, 0x20 ) );
#endif
		UpdateDisplayPortionEx( pf->pActImg
								  , x + pc->surface_rect.x
								  , y + pc->surface_rect.y
								  , w, h DBG_RELAY );
	}
}

//---------------------------------------------------------------------------

void IntelligentFrameUpdateAllDirtyControls( PSI_CONTROL pc DBG_PASS )
{
	{
		// Draw this now please?!
		PSI_PENDING_RECT upd;
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
		{
			_lprintf( DBG_RELAY )( "Intelligent update? %p", pc );
			lprintf( "pc = %p par = %p", pc, pc->parent );
		}
#endif
		upd.flags.bHasContent = 0;
		upd.flags.bTmpRect = 0;
		//lprintf( "doing update common..." );
		DoUpdateCommonEx( &upd, pc, FALSE, 0 DBG_RELAY);
		if( upd.flags.bHasContent )
		{
			PSI_CONTROL frame;
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "Updated all commons? %d,%d  %d,%d", upd.x,upd.y, upd.width, upd.height );
#endif
#ifdef BLAT_COLOR_UPDATE_PORTION
			BlatColor( GetFrame( pc )->Window
						, //pc->surface_rect.x
						 + upd.x
						, //pc->surface_rect.x
						 + upd.y
						, 5, 5, BASE_COLOR_ORANGE );
			TESTCOLOR=SetAlpha( BASE_COLOR_RED, 0x20 );
#endif
			// the frame may return NULL if the frame is bDestroy.
			if( frame = GetFrame( pc ) )
				DoUpdateFrame( frame
								 , upd.x, upd.y
								 , upd.width, upd.height
								 , FALSE
								  DBG_RELAY );
		}
	}
}

//---------------------------------------------------------------------------

void AddCommonUpdateRegionEx( PPSI_PENDING_RECT update_rect, int bSurface, PSI_CONTROL pc DBG_PASS )
#define AddCommonUpdateRegion(upd,surface,pc) AddCommonUpdateRegionEx( upd,surface,pc DBG_SRC )
{
	PSI_CONTROL parent;
	int32_t x, y;
	uint32_t wd, ht;
	if( !pc )
		return;
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		_lprintf(DBG_RELAY)( "Adding region.... (mayfbe should wait)" );
#endif
	if( !pc->parent && pc->flags.bRestoring )
	{
		if( g.flags.bLogDebugUpdate )
			lprintf( "no parent and not restoring." );
		wd = pc->rect.width;
		ht = pc->rect.height;
		x = 0;//pc->rect.x;
		y = 0;//pc->rect.y;
		pc->flags.bUpdateRegionSet = 0;
	}
	else if( pc->flags.bUpdateRegionSet )
	{
		if( g.flags.bLogDebugUpdate )
			lprintf( "someone already set the region... " );
		wd = pc->update_rect.width;
		ht = pc->update_rect.height;
		x = pc->update_rect.x + pc->surface_rect.x;
		y = pc->update_rect.y + pc->surface_rect.y;
		pc->flags.bUpdateRegionSet = 0;
	}
	else
	{
		if( g.flags.bLogDebugUpdate )
			lprintf( "parent and this is restoring " );
		if( bSurface )
		{
			//lprintf( "Computing control's surface rectangle." );
			wd = pc->surface_rect.width;
			ht = pc->surface_rect.height;
		}
		else
		{
			//lprintf( "Computing control's window rectangle." );
			wd = pc->rect.width;
			ht = pc->rect.height;
		}
		if( pc->parent && !pc->device )
		{
			x = pc->rect.x;
			y = pc->rect.y;
		}
		else
		{
			// if a control was created (or if we're using the bare
			// frame for the surface of something)
			// then don't include pc's offset since that is actually
			// a representation of the screen offset.
			x = 0;
			y = 0;
		}
		if( bSurface )
		{
			x += pc->surface_rect.x;
			y += pc->surface_rect.y;
		}
	}
	if( pc->parent )
	{
		if( g.flags.bLogDebugUpdate )
			lprintf( "control has parent..." );

		for( parent = pc->parent; parent /*&& parent->parent*/; parent = parent->parent )
		{
			x += ((parent->parent&&!parent->device)?parent->rect.x:0) + parent->surface_rect.x;
			y += ((parent->parent&&!parent->device)?parent->rect.y:0) + parent->surface_rect.y;
			if( g.flags.bLogDebugUpdate )
				lprintf( "control's parent makes x=%d and y=%d", x, y );
			if( parent->device )
				break;
		}
		//x += parent->surface_rect.x;
		//y += parent->surface_rect.y;
	}
	//else
   //   parent = pc;
   //lprintf( "Adding update region (%d,%d)-(%d,%d)", x, y, wd, ht );
	if( wd && ht )
	{
		if( update_rect->flags.bHasContent )
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "Adding (%d,%d)-(%d,%d) to (%d,%d)-(%d,%d)"
						 , x, y
						 , wd, ht
						 , update_rect->x, update_rect->y
						 , update_rect->width, update_rect->height
						 );
#endif
			if( x < update_rect->x )
			{
				update_rect->width += update_rect->x - x;
				update_rect->x = x;
			}
			if( ( x + (int32_t)wd ) > ( update_rect->x + (int32_t)update_rect->width ) )
				update_rect->width = ( (int32_t)wd + x ) - update_rect->x;

			if( y < update_rect->y )
			{
				update_rect->height += update_rect->y - y;
				update_rect->y = y;
			}
			if( y + (int32_t)ht > update_rect->y + (int32_t)update_rect->height )
				update_rect->height = ( y + (int32_t)ht ) - update_rect->y;
			//lprintf( (fs"result (%d,%d)-(%d,%d)")
		 //       , update_rect->x, update_rect->y
		 //       , update_rect->width, update_rect->height
			//		 );
		}
		else
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				_lprintf(DBG_RELAY)( "Setting (%d,%d)-(%d,%d)"
										 , x, y
										 , wd, ht
										 );
#endif
			update_rect->x = x;
			update_rect->y = y;
			update_rect->width = wd;
			update_rect->height = ht;
		}
		update_rect->flags.bHasContent = 1;
	}
}

//---------------------------------------------------------------------------

void SetUpdateRegionEx( PSI_CONTROL pc, int32_t rx, int32_t ry, uint32_t rw, uint32_t rh DBG_PASS )
{
	PSI_CONTROL parent;
	int32_t x, y;
	uint32_t wd, ht;
	if( !pc )
		return;


	//lprintf( "Computing control's surface rectangle." );
	wd = rw;
	ht = rh;

	if( pc->parent && !pc->device )
	{
		x = pc->rect.x;
		y = pc->rect.y;
	}
	else
	{
		// if a control was created (or if we're using the bare
		// frame for the surface of something)
		// then don't include pc's offset since that is actually
		// a representation of the screen offset.
		x = 0;
		y = 0;
	}
	x += pc->surface_rect.x;
	y += pc->surface_rect.y;
	x += rx;
	y += ry;

	if( pc->parent )
	{
		for( parent = pc->parent; parent /*&& parent->parent*/; parent = parent->parent )
		{
			x += ((parent->parent&&!parent->device)?parent->rect.x:0) + parent->surface_rect.x;
			y += ((parent->parent&&!parent->device)?parent->rect.y:0) + parent->surface_rect.y;
			if( parent->device )
				break;
		}
	}
	//else
   //   parent = pc;
   //lprintf( "Adding update region (%d,%d)-(%d,%d)", x, y, wd, ht );
	if( wd && ht )
	{
		pc->flags.bUpdateRegionSet = 1;
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				_lprintf(DBG_RELAY)( "Setting (%d,%d)-(%d,%d)"
										 , x, y
										 , wd, ht
										 );
#endif
			pc->update_rect.x = x;
			pc->update_rect.y = y;
			pc->update_rect.width = wd;
			pc->update_rect.height = ht;
		}
	}
}

//---------------------------------------------------------------------------

Image CopyOriginalSurfaceEx( PSI_CONTROL pc, Image use_image DBG_PASS )
{
	Image copy;
	if( !pc || !g.flags.allow_copy_from_render || !pc->parent || ( pc->BorderType & BORDER_FRAME ) )
	{
		return NULL;
	}

	if( pc->flags.bInitial )
	{
#ifdef DEBUG_TRANSPARENCY_SURFACE_SAVE_RESTORE
		lprintf( "Control has not drawn yet." );
#endif
		if( use_image )
			UnmakeImageFile( use_image );
		return NULL;
	}

	if( pc->flags.bParentCleaned && (pc->parent && !pc->parent->flags.bDirty ) && pc->flags.bParentUpdated )
	{
		pc->flags.bParentUpdated = 0;  // okay we'll have a new snapshot of the parent after this.
		if( !use_image )
		{
#ifdef DEBUG_TRANSPARENCY_SURFACE_SAVE_RESTORE
			lprintf( "Creating a new image.. %p", pc );
#endif
			copy = MakeImageFileEx( pc->rect.width, pc->rect.height DBG_RELAY );
		}
		else
		{
			// use smae image - typically pc->Original_image is set indirectly to this funciton's result
			// therefore there's only one ever, made if NULL and resized otherwise.
			// clean it up later...
#ifdef DEBUG_TRANSPARENCY_SURFACE_SAVE_RESTORE
			lprintf( "Using old image %p", pc );
#endif
			copy = use_image;
		}
		// if the sizes match already, resize does nothing.
		ResizeImage( copy, pc->rect.width, pc->rect.height );
		//lprintf( "################ COPY SURFACE ###################### " );
		BlotImage( copy, pc->Window, 0, 0 );
		//ClearImageTo( copy, BASE_COLOR_ORANGE );
#ifdef DEBUG_TRANSPARENCY_SURFACE_SAVE_RESTORE
		lprintf( "Copied old image %p", pc );
#endif
		return copy;
	}
#ifdef DEBUG_TRANSPARENCY_SURFACE_SAVE_RESTORE
	lprintf( "Parent was unclean, no image %p" );
#endif
	return NULL;
}

//---------------------------------------------------------------------------


// This routine actually sends the draw events to dirty rectangles.
static void DoUpdateCommonEx( PPSI_PENDING_RECT upd, PSI_CONTROL pc, int bDraw, int level DBG_PASS )
{
	int cleaned = 0;
	if( pc )
	{
#if LOCK_TEST
		PPHYSICAL_DEVICE device = GetFrame( pc )->device;
#endif
		if( pc->parent && !pc->device )
		{
			// okay surface rect of parent should be considered as 0,0.
			if( SUS_GT( pc->rect.x, IMAGE_COORDINATE, pc->parent->surface_rect.width, IMAGE_SIZE_COORDINATE )
			    || SUS_GT( pc->rect.y, IMAGE_COORDINATE, pc->parent->surface_rect.height, IMAGE_SIZE_COORDINATE )
			    || USS_LT( pc->rect.width, IMAGE_SIZE_COORDINATE, /*pc->parent->surface_rect.x*/-pc->rect.x, IMAGE_COORDINATE )
			    || USS_LT( pc->rect.height, IMAGE_SIZE_COORDINATE, /*pc->parent->surface_rect.y*/-pc->rect.y, IMAGE_COORDINATE )
			  )
			{
				if( g.flags.bLogDebugUpdate )
				{
					lprintf( "out of bounds? %d,%d %d,%d  %d,%d %d,%d", pc->original_rect.x, pc->original_rect.y, pc->original_rect.width, pc->original_rect.height, pc->parent->surface_rect.width, pc->parent->surface_rect.height, /*pc->parent->surface_rect.x*/-pc->original_rect.x, /*pc->parent->surface_rect.y*/-pc->original_rect.y );
				}
				return;
			}
		}
#if DEBUG_UPDAATE_DRAW > 2
		if( g.flags.bLogDebugUpdate )
			_lprintf(DBG_RELAY)( ">>Control updating %p(parent:%p,child:%p)", pc, pc->parent, pc->child );
#endif
		if( pc->flags.bNoUpdate /*|| !GetImageSurface(pc->Surface)*/ )
		{
#if DEBUG_UPDAATE_DRAW > 2
			if( g.flags.bLogDebugUpdate )
				lprintf( "Control update is disabled" );
#endif
			return;
		}
#if DEBUG_UPDAATE_DRAW > 3
		if( g.flags.bLogDebugUpdate )
		{
			// might filter this to just if dirty, we get called a lot without dirty controls
			lprintf( "Control %p(%s) is %s and parent is %s", pc, pc->pTypeName, pc->flags.bDirty?"SMUDGED":"clean", pc->flags.bParentCleaned?"cleaned to me":"dirty to me" );
			// again might filter to just forced...
			_xlprintf(LOG_NOISE DBG_RELAY )( ">>do draw... %p %p %s %s", pc, pc->child
			                               , bDraw?"FORCE":"..."
			                               , pc->flags.bCleaning?"CLEANING":"cleanable");
		}
#endif
#if LOCK_TEST
		if( device )
			LockRenderer( device->pActImg );
#endif
		//#endif
		if( !pc->flags.bCleaning )
		{
#if DEBUG_UPDAATE_DRAW > 2
			if( g.flags.bLogDebugUpdate )
				_xlprintf(1 DBG_RELAY)( ">>Begin control %p update forced?%s", pc, bDraw?"Yes":"No" );
#endif
			pc->flags.bCleaning = 1;
		retry_update:
			if( pc->parent && !pc->device )
			{
				// if my parent is initial, become initial also...
				//lprintf( " Again, relaying my initial status..." );
				if( pc->flags.bInitial != pc->parent->flags.bInitial )
				{
					pc->flags.bInitial = pc->parent->flags.bInitial;
					if( !pc->flags.bHidden )
						InvokeControlRevealed( pc );
				}

				// also, copy hidden status... can't be shown within a hidden parent.
				if( !pc->flags.bHiddenParent )
				{
					// if this control itself is the hidden control
					// (top level hidden controls are ParentHidden)
					// don't copy the parent's visibility - cause it probably
					// is visible.
					pc->flags.bHidden = pc->parent->flags.bHidden;
				}
			}
			/* this previsouly tested if bInitial... but that only counts for the top level... */
			if( !pc->flags.bNoUpdate && ((pc->parent&&!pc->device)?1:!pc->flags.bInitial) && ( (pc->flags.bDirty || pc->flags.bDirtied) || bDraw ) && !pc->flags.bHidden )
			{
#if DEBUG_UPDAATE_DRAW > 2
				if( g.flags.bLogDebugUpdate )
					lprintf( "Control draw %p %s parent_clean?%d transparent?%d"
							 , pc, pc->pTypeName
							 , pc->flags.bParentCleaned
							 , pc->flags.bTransparent );
#endif
				if( !g.flags.always_draw
				   || ( g.flags.always_draw && !IsImageTargetFinal( pc->Window ) )
					&& g.flags.allow_copy_from_render )
				{
					if( ( ((pc->parent&&!pc->device) && pc->parent->flags.bDirty )
					    || pc->flags.bParentCleaned )
					  && pc->flags.bTransparent )//&& pc->flags.bFirstCleaning )
					{
						Image OldSurface;
						OldSurface = CopyOriginalSurface( pc, pc->OriginalSurface );

						if( OldSurface )
						{
#ifdef DEBUG_UPDAATE_DRAW
							if( g.flags.bLogDebugUpdate )
								_lprintf(DBG_RELAY)( "--------------- Successfully copied new background original image" );
#endif
							pc->OriginalSurface = OldSurface;
						}
						else
							if( pc->OriginalSurface )
							{
#ifdef DEBUG_UPDAATE_DRAW
								if( g.flags.bLogDebugUpdate )
									_xlprintf(LOG_NOISE DBG_RELAY)( "--------------- Restoring prior image (didn't need a new image)" );
								if( g.flags.bLogDebugUpdate )
									lprintf( "Restoring orignal background... " );
#endif
								BlotImage( pc->Window, pc->OriginalSurface, 0, 0 );
								pc->flags.bParentCleaned = 1;
								pc->flags.children_cleaned = 0;
							}
					}
					else
					{
						if( pc->OriginalSurface )
						{
#ifdef DEBUG_UPDAATE_DRAW
							if( g.flags.bLogDebugUpdate )
								_xlprintf(LOG_NOISE DBG_RELAY)( "--------------- Restoring prior image" );
							if( g.flags.bLogDebugUpdate )
								lprintf( "Restore original background..." );
#endif
							BlotImage( pc->Window, pc->OriginalSurface, 0, 0 );
							pc->flags.bParentCleaned = 1;
							pc->flags.children_cleaned = 0;
						}
					}
				}
				pc->flags.bFirstCleaning = 0;
			}
			// dirty, draw, not hidden... but also
			// bInitial still invokes first draw.
			if( !pc->flags.bNoUpdate && ( g.flags.always_draw || pc->flags.bDirty || bDraw ) && !pc->flags.bHidden )
			{
				Image current = NULL;
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate ) {
					_lprintf( DBG_RELAY )( "Invoking a draw self for %p level %d", pc, level );
				}
#endif
				if( pc->flags.bDestroy )
				{
#if LOCK_TEST
					if( device )
						UnlockRenderer( device->pActImg );
#endif
					return;
				}
				if( pc->flags.bTransparent
					&& !pc->flags.bParentCleaned
					&& (pc->parent&&!pc->device)
					)
				{
#ifdef DEBUG_UPDAATE_DRAW
					if( g.flags.bLogDebugUpdate )
						lprintf( "(RECURSE UP!)Calling parent inline - wasn't clean, isn't clean, try and get it clean..." );
#endif
					// could adjust the clipping rectangle...
					DoUpdateCommonEx( upd, pc->parent, TRUE, level+1 DBG_RELAY );

					if( !upd->flags.bHasContent )
					{
						//  control did not draw...
						//lprintf( " Expected handling of this condition... Please return FALSE, and abort UpdateDIsplayPortion? Return now, leaving the rect without content?" );
						//DebugBreak();
					}
					if( !pc->flags.bParentCleaned || ( ( pc->parent && !pc->device )?pc->parent->flags.bDirty:0))
					{
						//DebugBreak();
						lprintf( "Aborting my update... waiting for container to get his update done" );
#if LOCK_TEST
						if( device )
							UnlockRenderer( device->pActImg );
#endif
						pc->flags.bCleaning = 0;
						return;
					}

					if( pc->flags.bTransparent )
					{
						//lprintf( "COPYING SURFACE HERE!?" );
						// we should be drawing when the parent does his thing...
						if( g.flags.allow_copy_from_render )
							pc->OriginalSurface = CopyOriginalSurface( pc, pc->OriginalSurface );
						else
							pc->OriginalSurface = NULL;
					}
				}

				pc->draw_result = 0;
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					_lprintf(DBG_RELAY)( " --- INVOKE DRAW (get region) --- %s ", pc->pTypeName );
#endif
				{
#if LOCK_TEST
					PPHYSICAL_DEVICE device = GetFrame( pc )->device;
					if( device )
						LockRenderer( device->pActImg );
#endif
					// this causes a lock in that layer.?
					ResetImageBuffers( pc->Surface, FALSE );
					InvokeDrawMethod( pc, _DrawThySelf, ( pc ) );
				}

#ifdef DEBUG_UPDAATE_DRAW
				// if it didn't draw... then why do anything?
				if( g.flags.bLogDebugUpdate )
					lprintf( "draw result is... %d", pc->draw_result );
#endif

				//if( current )
				{
					if( !pc->draw_result )
					{
						//pc->flags.bParentCleaned = 0; // has now drawn itself, and we must assume that it's not clean.
 						//BlotImage( pc->Window, current, 0, 0 );
					}
					else
					{
#ifdef DEBUG_UPDAATE_DRAW
						if( g.flags.bLogDebugUpdate )
							lprintf( "Parent is no longer cleaned...." );
#endif
						pc->flags.bCleanedRecently = 1;
						//pc->flags.bParentCleaned = 0; // has now drawn itself, and we must assume that it's not clean.
						pc->flags.children_cleaned = 0;
					}

					//UnmakeImageFile( current);
				}


				// better clear this flag after so that a smudge during
				// a dumb control doesn't make us loop...
				// though I suppose some other control could cause us to draw again?
				// well leaving it set during will cause smudge to do what exactly


				pc->flags.bDirty = FALSE;
				//lprintf( "Invoked a draw self" );
				// the outermost border/frame will be drawn
				// from a different place... this one only needs to
				// worry aobut child region borders after telling them to
				// draw - to enforce cleanest bordering...
				if( g.flags.always_draw
					|| ( pc->parent && !pc->parent->flags.children_cleaned )
					|| ( pc->flags.bParentCleaned )
					)
					if( pc->DrawBorder )  // and initial?
					{
#ifdef DEBUG_BORDER_DRAWING
						lprintf( "Drawing border here too.." );
#endif
						pc->DrawBorder( pc );
					}

#if DEBUG_UPDAATE_DRAW > 2
				if( g.flags.bLogDebugUpdate )
				{
					if( upd->flags.bHasContent )
						_lprintf(DBG_RELAY)( "Added update region %d,%d %d,%d", upd->x ,upd->y, upd->width, upd->height );
					else
						_lprintf(DBG_RELAY)( "No prior content in update rect..." );
				}
#endif
				// okay hokey logic here...
				// if not transparent - go
				// (else IS transparent, in which case if draw_result, go )
				//   ELSE don't add

				if( !pc->flags.bTransparent || pc->draw_result )
				{
					AddCommonUpdateRegion( upd, FALSE, pc );
				}
#if DEBUG_UPDAATE_DRAW > 2
				if( g.flags.bLogDebugUpdate )
				{
					if( upd->flags.bHasContent )
						_lprintf(DBG_RELAY)( "Added update region %d,%d %d,%d", upd->x ,upd->y, upd->width, upd->height );
				}
#endif
				cleaned = 1;
				{
					PSI_CONTROL child;
					//lprintf( "pc is %p and first child is %p", pc, pc->child );
					for( child = pc->child; child; child = child->next )
					{
						if( pc->draw_result )
						{
							// if it didn't draw, then probably the prior snapshot is still valid
							child->flags.bParentUpdated = 1; // set so controls grab new snapshots
						}
						child->flags.bParentCleaned = 1; // has now drawn itself, and we must assume that it's not clean.
#if DEBUG_UPDAATE_DRAW > 2
						if( g.flags.bLogDebugUpdate )
							lprintf( "marking on child %p parent %p(%p) is %s;%s", child, pc, child->parent, cleaned?"CLEANED":"UNCLEAN", child->parent->flags.bDirty?"DIRTY":"not dirty" );
#endif
					}
				}
				//pc->flags.bDirty = 0;
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( "And now it has been cleaned..." );
#endif
			}
			else
			{
				//cleaned = 1; // well, lie here... cause we're already clean?
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( "Not invoking draw self." );
#endif
				//#if 0
#if DEBUG_UPDAATE_DRAW > 3
				// general logging of the current status of the control
				// at this point the NORMAL status is clean, visible, no force, and drawing proc.
				if( g.flags.bLogDebugUpdate )
					lprintf( "control %p is %s %s %s %s", pc, pc->flags.bDirty?"dirty":"clean"
							 , bDraw?"force":""
							 , pc->flags.bHidden?"hidden!":"visible..."
							 , pc->_DrawThySelf?"drawingproc":"NO DRAW PROC" );
#endif
//#endif
			}
			// check for dirty children.  That we over-drew ourselves...
//		check_for_dirty_children: ;
			if( !pc->flags.children_cleaned )
			{
				PSI_CONTROL child;
				for( child = pc->child; child; child = child->next )
				{
					// that we are ourselved drawn implies that
					// our max bound will be updated when children finish.
					// I drew myself, must draw all children.
					//lprintf( "doing a child update..." );
#if DEBUG_UPDAATE_DRAW > 2
					if( g.flags.bLogDebugUpdate )
						lprintf( "updating child %p parent %p is %s;%s", child, pc, cleaned?"CLEANED":"UNCLEAN", child->parent->flags.bDirty?"DIRTY":"not dirty" );
#endif
					//lprintf( "Do update region %d,%d %d,%d", upd->x ,upd->y, upd->width, upd->height );
					//child->flags.bParentCleaned = 1; // has now drawn itself, and we must assume that it's not clean.
					DoUpdateCommonEx( upd, child, cleaned, level+1 DBG_RELAY );
				}
			}
			if( pc->flags.bDirtied )
			{
				pc->flags.bDirtied = 0;
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( "Recovered dirty that was set while we were cleaning... going back to draw again." );
#endif
				goto retry_update;
			}
			{
				PSI_CONTROL pcTemp = pc;
				for( pcTemp = pc; pcTemp; pcTemp = pcTemp->parent )
					// after all children have updated, draw decorations (edit hotspots, cover animations...)
					InvokeMethod( pcTemp, _DrawDecorations, (pcTemp, pc) );
			}

#if LOCK_TEST
			{
				PPHYSICAL_DEVICE device = GetFrame( pc )->device;
				if( device )
					UnlockRenderer( device->pActImg );
			}
#endif

			pc->flags.children_cleaned = 1;
			pc->flags.bCleaning = 0;
			pc->flags.bFirstCleaning = 1;
			//pc->flags.bParentCleaned = 0; // has now drawn itself, and we must assume that it's not clean.
		}
		else
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				_xlprintf(1 DBG_RELAY)( "Already Cleaning!!!!!  (THIS IS A FATAL LOCK POTENTIAL)" );
#endif
			if( pc->NotInUse )
			{
				// it's in clean, and did a 'releasecommonuse'
				// this means that it wants everything within itself
				// to draw, at which time it will do it's own draw.
				// there's going to be a hang though, when the final
				// update to display happens... need to catch that.
				cleaned = 1; // lie.  The parent claims it finished cleaning
				// clear this, cause we're no longer drawing within the parent
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( "Parent is no longer cleaned..." );
#endif
				pc->flags.bParentCleaned = 0; // has now drawn itself, and we must assume that it's not clean.
				pc->flags.children_cleaned = 1;
				{
					PSI_CONTROL child;
					for( child = pc->child; child; child = child->next )
					{
						// that we are ourselved drawn implies that
						// our max bound will be updated when children finish.
						// I drew myself, must draw all children.
#ifdef DEBUG_UPDAATE_DRAW
						if( g.flags.bLogDebugUpdate )
							lprintf( "***doing a child update... to %s", cleaned?"CLEAN":"UNCLEAN" );
#endif
						child->flags.bParentCleaned = cleaned; // has now drawn itself, and we must assume that it's not clean.
						//lprintf( "Do update region %d,%d %d,%d", upd->x ,upd->y, upd->width, upd->height );
						//child->flags.bParentCleaned = 1; // has now drawn itself, and we must assume that it's not clean.
						DoUpdateCommonEx( upd, child, cleaned, level+1 DBG_SRC );
					}
				}
				//pc->flags.bParentCleaned = 0; // has now drawn itself, and we must assume that it's not clean.
				//pc->flags.children_cleaned = 1;
			}
#if DEBUG_UPDAATE_DRAW > 2
			if( g.flags.bLogDebugUpdate )
				_xlprintf(1 DBG_RELAY)( "Control %p already drawing itself!?", pc );
#endif
		}
#if LOCK_TEST
		if( device )
			UnlockRenderer( device->pActImg );
#endif
	}
#ifdef DEBUG_UPDAATE_DRAW
	else
		if( g.flags.bLogDebugUpdate )
			_xlprintf(1 DBG_RELAY)( "NULL control told to update!" );
#endif
}

static int ChildInUse( PSI_CONTROL pc, int level )
{
	int n;
	while( pc )
	{
		if( pc->InUse )
			return TRUE;
		n = ChildInUse( pc->child, level+1 );
		if( n )
			return n;
		if( level )
			pc = pc->next;
		else
			break;
	}
	return FALSE;
}

//---------------------------------------------------------------------------

void SmudgeCommonEx( PSI_CONTROL pc DBG_PASS )
{
	if( !pc )
		return;
	if( pc->flags.bDestroy || !pc->Window )
		return;

	// if( pc->flags.bTransparent && pc->parent && !pc->parent->flags.bDirty )
	//    SmudgeCommon( pc->parent );
	if( g.flags.always_draw )
	{
		if( !pc->flags.bDestroy )
		{
			PSI_CONTROL frame = GetFrame( pc );
			PPHYSICAL_DEVICE device = frame?frame->device:NULL;
			if( device )
				MarkDisplayUpdated( device->pActImg );
		}
		if( g.flags.bLogDebugUpdate )
			_lprintf(DBG_RELAY)( "%p(%s) wanted to draw...", pc, pc->pTypeName );
		return;
	}
	else {
		LOGICAL schedule_only = FALSE;
#if DEBUG_UPDAATE_DRAW > 0
		if( g.flags.bLogDebugUpdate )
			_lprintf( DBG_RELAY )("Smudge %p %s", pc, pc->pTypeName ? pc->pTypeName : "NoTypeName");
#endif
		{
			PSI_CONTROL parent;
			for( parent = pc; parent; parent = parent->parent )
			{
				if( parent->flags.bNoUpdate || parent->flags.bHidden )
				{
#if DEBUG_UPDAATE_DRAW > 3
					if( g.flags.bLogDebugUpdate )
						lprintf( "a control %p(%d) (self, or some parent %p(%d)) has %s or %s  (marks me as dirty anhow, but doesn't attempt anything further)"
							, pc, pc->nType, parent, parent->nType
							, parent->flags.bNoUpdate ? "noupdate" : "..."
							, parent->flags.bHidden ? "hidden" : "..."
							);
#endif
					pc->flags.bDirty = 1;
					{
						PSI_CONTROL child;
						for( child = pc->child; child; child = child->next )
						{
							child->flags.bParentCleaned = 0; // has now drawn itself, and we must assume that it's not clean.
						}
					}
					// otherwise we need to schedule the drawing...
					if( g.flags.allow_threaded_draw )
						return;
					else
						schedule_only = TRUE;
				}
			}
		}
		if( !g.flags.allow_threaded_draw && !IsThisThread( g.updateThread ) )
		{
			PSI_CONTROL frame = GetFrame( pc );
			PPHYSICAL_DEVICE device = frame?frame->device:NULL;
			if( device )
			{
#if DEBUG_UPDAATE_DRAW > 0
				if( g.flags.bLogDebugUpdate )
					_lprintf(DBG_RELAY)( "Add to dirty controls... Smudge %p %s", pc, pc->pTypeName?pc->pTypeName:"NoTypeName" );
#endif
				if( FindLink( &device->pending_dirty_controls, pc ) == INVALID_INDEX )
					AddLink( &device->pending_dirty_controls, pc );
				if( !schedule_only && !device->flags.sent_redraw && device->pActImg )
				{
					//lprintf( "Send redraw to self.... draw controls in pending_dirty_controls" );
					//device->flags.sent_redraw = 1;
					//lprintf( " -----  parent update Redraw ----" );
					Redraw( device->pActImg );
				}
			}
			return;
		}
		else
		{
			if( pc->flags.bDirty || pc->flags.bCleaning )
			{
				if( pc->flags.bCleaning )
				{
					uint32_t tick = timeGetTime();
					// something changed, and we'll have to draw that control again... as soon as it's done cleaning actually.
					pc->flags.bDirtied = 1;
					while( pc->flags.bCleaning && pc->flags.bDirtied && ((tick + 500) < timeGetTime()) )
						WakeableSleep( 10 );
					if( !pc->flags.bCleaning )
						if( pc->flags.bDirtied )
							pc->flags.bDirty = 1;
				}
#if DEBUG_UPDAATE_DRAW > 3
				if( g.flags.bLogDebugUpdate )
					_xlprintf( LOG_LEVEL_DEBUG DBG_RELAY )("%s %s %s %p"
						, pc->flags.bDirty ? "already smudged" : ""
						, (pc->flags.bDirty && pc->flags.bCleaning) ? "and" : ""
						, pc->flags.bCleaning ? "in process of cleaning..." : ""
						, pc);
#endif

				//Sleep( 10 );
				   //return;
			}
			else // if( !pc->flags.bDirty )
			{
				PSI_CONTROL parent;
#if DEBUG_UPDAATE_DRAW > 3
				if( g.flags.bLogDebugUpdate )
					lprintf( "not dirty, and not cleaning" );
#endif

#ifdef DEBUG_UPDAATE_DRAW
				//if( pc->parent && pc->flags.bTransparent )
				//   SmudgeCommon( pc->parent );
				for( parent = pc; parent /*&& parent->flags.bTransparent*/ &&
					!parent->InUse &&
					!parent->flags.bDirty; parent = parent->parent )
				{
					if( g.flags.bLogDebugUpdate )
						_xlprintf( LOG_LEVEL_DEBUG DBG_RELAY ) ("%s %p is %s and %s %s"
							, (parent == pc) ? "self" : "parent"
							, parent
							, parent->InUse ? "USED" : "Not Used"
							, parent->flags.bTransparent ? "Transparent" : "opaque"
							, parent->flags.bChildDirty ? "has Dirty Child" : "children clean");
					//parent->flags.bChildDirty = 1;
				}
#endif

				// a parent is in use (misleading - parent may be self...)
				// - that means that
				// an event is dispatched, and when that event is complete
				// it will have all children checked for dirt.

#ifdef DEBUG_UPDAATE_DRAW
				{
					if( g.flags.bLogDebugUpdate )
						_xlprintf( LOG_NOISE DBG_RELAY )("marking myself dirty. %p", pc);
				}
#endif
				pc->flags.bDirty = 1;
			}
		}
		if( pc->flags.bOpenGL )
		{
			UpdateDisplay( GetFrameRenderer( pc ) );
			return;
		}
		// actually when it comes time to do the update
		// the children's draw is forced-run because the
		// parent updated, and therefore obviously oblitereted
		// the region the child had, the child therefore NEEDS to draw.

		// if dirty, but nothings in use...
		// then I guess we get to clear stuff out.
		//if( !pc->InUse && !ChildInUse( pc, 0 ) )
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "control is not in use... (smudge logic) Initial=%d ", pc->flags.bInitial );
#endif
			if( !pc->flags.bInitial )
			{
				IntelligentFrameUpdateAllDirtyControls( pc DBG_RELAY );
			}
#ifdef DEBUG_UPDAATE_DRAW
			else
				if( g.flags.bLogDebugUpdate )
					lprintf( "pc->flags.bInitial is true..." );
#endif
		}
	}
}

//---------------------------------------------------------------------------

void DoDumpFrameContents( int level, PSI_CONTROL pc )
{
	while( pc )
	{
		lprintf( "%*.*s" "Control %d(%d)%p at (%" _32fs ",%" _32fs ")-(%" _32f ",%" _32f ") (%" _32fs ",%" _32fs ")-(%" _32f ",%" _32f ") (%s %s %s %s) '%s' [%s]"
		       , level*3,level*3,"----------------------------------------------------------------"
		       , pc->nID, pc->nType, pc
		       , pc->rect.x, pc->rect.y, pc->rect.width, pc->rect.height
		       , pc->surface_rect.x
		       , pc->surface_rect.y
		       , pc->surface_rect.width
		       , pc->surface_rect.height
		       , pc->flags.bTransparent?"transparent":"t"
		       , pc->flags.bDirty?"dirty":"d"
		       , pc->flags.bNoUpdate?"NoUpdate":"nu"
		       , pc->flags.bHidden?"hidden":"h"
		       , pc->caption.text?GetText(pc->caption.text):""
		       , pc->pTypeName
		       );

		if( pc->child )
		{
			//lprintf( "%*.*s", level*2,level*2,"                                                                                                          " );
			DoDumpFrameContents( level + 1, pc->child );
		}
		pc = pc->next;
	}
}

void DumpFrameContents( PSI_CONTROL pc )
{
   DoDumpFrameContents( 0, GetFrame( pc ) );
}

//---------------------------------------------------------------------------

PSI_PROC( void, UpdateCommonEx )( PSI_CONTROL pc, int bDraw DBG_PASS )
{
	PSI_PENDING_RECT upd;
	upd.flags.bTmpRect = 0;
	upd.flags.bHasContent = 0;

	if( !(pc->flags.bNoUpdate || pc->flags.bHidden) )
	{
		PSI_CONTROL frame = GetFrame( pc );
		if( frame ) // else we're being destroyed probably...
		{
#if DEBUG_UPDAATE_DRAW > 2
			if( g.flags.bLogDebugUpdate )
				_xlprintf(LOG_NOISE DBG_RELAY )( "Updating common ( which invokes frame... )%p %p", pc, frame );
#endif
			// go up, check all
			{
				//void DumpFrameContents( PSI_CONTROL pc );
				//DumpFrameContents( frame );
			}
			DoUpdateCommonEx( &upd, pc, bDraw, 0 DBG_RELAY );
			if( upd.flags.bHasContent )
			{
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( "Updated all commons? flush display %d,%d  %d,%d", upd.x,upd.y, upd.width, upd.height );
#endif
#ifdef BLAT_COLOR_UPDATE_PORTION
				TESTCOLOR=SetAlpha( BASE_COLOR_GREEN, 0x80 );
#endif
				DoUpdateFrame( frame
								 , upd.x, upd.y
								 , upd.width, upd.height
								 , FALSE
								  DBG_SRC );
			}
		}
	}
}

//---------------------------------------------------------------------------
static int OnCreateCommon( "Frame" )( PSI_CONTROL pc )
{
	// cannot fail create frame - it's a simple control
	// later - if DisplayFrame( pc ) creates a physical display
	// for such a thing, and DIsplayFrameOver if that should be
	// modally related to another contorl/frame
	return 1;
}


CONTROL_REGISTRATION frame_controls = { "Frame", { { 320, 240 }, 0, BORDER_NORMAL }
};

PRIORITY_PRELOAD( register_frame_control, PSI_PRELOAD_PRIORITY )
{
   DoRegisterControlEx( &frame_controls, sizeof( frame_controls ) );
}

//---------------------------------------------------------------------------
static void LinkInNewControl( PSI_CONTROL parent, PSI_CONTROL elder, PSI_CONTROL child );

PROCEDURE RealCreateCommonExx( PSI_CONTROL *pResult
									  , PSI_CONTROL pContainer
										// if not typename, must pass type ID...
									  , CTEXTSTR pTypeName
									  , uint32_t nType
										// position of control
									  , int x, int y
									  , int w, int h
									  , uint32_t nID
									  , CTEXTSTR pIDName
										// ALL controls have a caption...
									  , CTEXTSTR text
										// fields in this override the defaults...
										// if not 0.
									  , uint32_t ExtraBorderType
									  , int bCreate // if !bCreate, return reload type
										// if this is reloaded...
										//, PTEXT parameters
										//, va_list args
										DBG_PASS )
{
	TEXTCHAR mydef[256];
	PCLASSROOT root;
	PSI_CONTROL pc = NULL;
	uint32_t BorderType;

#ifdef USE_INTERFACES
	GetMyInterface();

	if( !g.MyImageInterface )
	{
		if( pResult )
			(*pResult) = NULL;
		return NULL;
	}
#endif
	if( pTypeName )
		tnprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY "/control/%s", pTypeName );
	else
		tnprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY "/control/%" _32f , nType );
	root = GetClassRoot( mydef );
	if( pTypeName )
		nType = (int)(uintptr_t)GetRegisteredValueExx( root, NULL, "type", TRUE );
	else
		pTypeName = GetRegisteredValueExx( root, NULL, "type", FALSE );

	pc = (PSI_CONTROL)AllocateEx( sizeof( FR_CT_COMMON ) DBG_RELAY );
	MemSet( pc, 0, sizeof( FR_CT_COMMON ) );
	pc->class_root = root;
	//pc->basecolors = basecolor( pContainer );
	{
		uint32_t size = GetRegisteredIntValue( root, "extra" );
		if( size )
		{
			POINTER data = Allocate( size );
			if( data )
				MemSet( data, 0, size );
			SetControlData( POINTER, pc, data );
		}
	}
	if( !w )
		w = GetRegisteredIntValue( root, "width" );
	if( !h )
		h = GetRegisteredIntValue( root, "height" );
	// save the orginal width/height/size
	// (pre-scaling...)
	pc->original_rect.x = x;
	pc->original_rect.y = y;
	pc->original_rect.width = w;
	pc->original_rect.height = h;
#ifdef DEFAULT_CONTROLS_TRANSPARENT
	pc->flags.bTransparent = 1;
#endif
	pc->flags.bParentCleaned = 1;
	BorderType = (int)(uintptr_t)GetRegisteredValueExx( root, NULL, "border", TRUE );
	BorderType |= ExtraBorderType;
	//lprintf( "BorderType is %08x", BorderType );
	if( !(BorderType & BORDER_FIXED) )
	{
		PFRACTION sx, sy;
		GetCommonScale( pContainer, &sx, &sy );
		x = ScaleValue( sx, x );
		w = ScaleValue( sx, w );
		y = ScaleValue( sy, y );
		h = ScaleValue( sy, h );
	}
	if( pIDName )
	{
		pc->nID = GetResourceID( pContainer, pIDName, nID );
		pc->pIDName = StrDup( pIDName );
	}
	else
	{
		pc->nID = nID;
		pc->pIDName = GetResourceIDName( pTypeName, nID );

	}
	pc->nType = nType;
	pc->pTypeName = SaveText( pTypeName );

	if( pContainer )
	{
		pc->Window = MakeSubImage( pContainer->Surface, x, y, w, h );
		if( !pc->Window )
		{
			DebugBreak();
		}
	}
	else
	{
		// need some kinda window here...
		pc->Window = BuildImageFile( NULL, w, h );
		if( !pc->Window )
		{
			DebugBreak();
		}
		// the surface will not exist until it is mounted
		// on a surface?  as of yet, we do not have invisible
		// memory-only dialogs(frames) but there comes a time
		// in all controls life when it needs its own backing
		// with real pixel data.
		// MakeImageFile( w, h );
	}

	// normal rect of control - post scaling, true coords within container..
	pc->rect.x = x;
	pc->rect.y = y;
	pc->rect.width = w;
	pc->rect.height = h;
	//lprintf( "Set default fraction 1/1" );
	SetFraction( pc->scalex, 1, 1 );
	SetFraction( pc->scaley, 1, 1 );
	pc->flags.bInitial = 1;

	//else
	//	pc->caption.font = g.default_font;

	// from here forward, root and mydef reference the RTTI of the control...
	//
	tnprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY "/control/%" _32f "/rtti", nType );
	root = GetClassRoot( mydef );
	SetCommonDraw( pc, GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,int,"draw",(PSI_CONTROL)));
	SetCommonDrawDecorations( pc, GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"decoration_draw",(PSI_CONTROL,PSI_CONTROL)));
	SetCommonMouse( pc, GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,int,"mouse",(PSI_CONTROL,int32_t,int32_t,uint32_t)));
	SetCommonKey( pc, GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,int,"key",(PSI_CONTROL,uint32_t)));
	pc->Destroy        = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"destroy",(PSI_CONTROL));
	pc->CaptionChanged = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"caption_changed",(PSI_CONTROL));
	pc->ChangeFocus    = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,int,"focus_changed",(PSI_CONTROL,LOGICAL));
	pc->AddedControl   = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"add_control",(PSI_CONTROL,PSI_CONTROL));
	AddCommonAcceptDroppedFiles( pc, GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,LOGICAL,"drop_accept",(PSI_CONTROL,CTEXTSTR,int32_t,int32_t)) );
	pc->Resize         = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"resize",(PSI_CONTROL,LOGICAL));
	pc->Move         = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"position_changing",(PSI_CONTROL,LOGICAL));
	pc->Rescale        = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"rescale",(PSI_CONTROL));
	pc->BorderDrawProc = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"border_draw",(PSI_CONTROL,Image));
	pc->Rollover = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"rollover",(PSI_CONTROL,LOGICAL));
	pc->FontChange = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL, void, "font_change", (PSI_CONTROL) );
	if( !pContainer || ( ExtraBorderType & BORDER_FRAME ) )
	{
		// define default border.
		if( !g.flags.system_color_set )
			TryLoadingFrameImage();
		pc->border = g.DefaultBorder;
	}

	if( pc->BorderDrawProc )
	{
		pc->BorderMeasureProc = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"border_measure",(PSI_CONTROL,int*,int*,int*,int*));
		pc->BorderType = BORDER_USER_PROC | ( BorderType & ~BORDER_TYPE );
		SetDrawBorder( pc );  // sets real draw proc
	}
	else
	{
		pc->BorderType = ~BorderType;  // this time, force the setting of the border... even if it's '0' or NONE
		SetCommonBorder( pc, BorderType ); // updates the border proc...
	}

	//pc->PosChanging    = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"position_changing",(PSI_CONTROL,LOGICAL));
	pc->BeginEdit      = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"begin_frame_edit",(PSI_CONTROL));
	pc->EndEdit        = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"end_frame_edit",(PSI_CONTROL));
	pc->DrawCaption    = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,"draw_caption",(PSI_CONTROL,Image));
	if( pc->DrawCaption )
		pc->pCaptionImage = MakeSubImage( pc->Window, 0, 0, 0, 0 ); // this will get resized later

	// creates
	pc->flags.bInitial = 1;
	pc->flags.bDirty = 1;
	pc->flags.bCreating = 1;
	// have to set border type to non 0 here; in case border includes FIXED and the scaling in SetFont should be skipped.
	if( !pContainer /*pc->nType == CONTROL_FRAME*/ && g.default_font )
	{
		SetCommonFont( pc, g.default_font );
	}
	pc->flags.bSetBorderType = 0;

	pc->CaptionChanged = NULL; // don't send this event, and uninitialized(yet)
	pc->ChangeFocus = NULL; // don't send this event, and uninitialized(yet)

	SetControlText( pc, text );
	if( pContainer )
	{
		LinkInNewControl( pContainer, NULL, pc );
		pContainer->InUse++;
		//lprintf( "Added one to use.." );
		if( !pContainer->flags.bDirty )
			SmudgeCommon( pc );
		pContainer->InUse--;
		//lprintf( "Removed one to use.." );
	}

	if( pResult )
		*pResult = pc;

	if( !bCreate )
	{
		int (CPROC*Restore)(PSI_CONTROL,PTEXT);
		// allow init to return FALSE to destroy the control...
		Restore = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,int,"load",(PSI_CONTROL,PTEXT));
		if( Restore )
			return (PROCEDURE)Restore;
	}
	else
	{
		int (CPROC*Init)(PSI_CONTROL,va_list);
		// allow init to return FALSE to destroy the control...
		Init = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,int,"init",(PSI_CONTROL,va_list));
		if( Init )
			return (PROCEDURE)Init;
	}
	return NULL; //pc;
}

//---------------------------------------------------------------------------

#define CreateCommonExx(pc,pt,nt,x,y,w,h,id,cap,ebt,p,ep) CreateCommonExxx(pc,pt,nt,x,y,w,h,id,NULL,cap,ebt,p,ep)
PSI_PROC( PSI_CONTROL, CreateCommonExxx)( PSI_CONTROL pContainer
													 , CTEXTSTR pTypeName
													 , uint32_t nType
													 , int x, int y
													 , int w, int h
													 , uint32_t nID
													 , CTEXTSTR pIDName
													 , CTEXTSTR caption
													 , uint32_t ExtraBorderType
													 , PTEXT parameters
													 , POINTER extra_param
													  DBG_PASS );

//---------------------------------------------------------------------------

PSI_PROC( PSI_CONTROL, CreateFrame )( CTEXTSTR caption
										  , int x, int y
										  , int w, int h
										  , uint32_t BorderTypeFlags
										  , PSI_CONTROL hAbove )
{
	PSI_CONTROL pc;
	//lprintf( "Creating a frame at %d,%d %d,%d", x, y, w, h );
#ifdef USE_INTERFACES
	GetMyInterface(); // macro with builtin quickcheck
#endif
	pc = CreateCommonExx( NULL // hAbove//(BorderTypeFlags & BORDER_WITHIN)?hAbove:NULL
							  , NULL, CONTROL_FRAME
							  , x, y
							  , w, h
							  , -1
							  , caption
							  , BorderTypeFlags
							  , NULL
							  , NULL
								DBG_SRC );
	if( !(BorderTypeFlags & BORDER_WITHIN ) )
		pc->parent = hAbove;
	SetCommonBorder( pc, BorderTypeFlags
				| (( BorderTypeFlags & BORDER_CAPTION_NO_CLOSE_BUTTON )?0:BORDER_CAPTION_CLOSE_BUTTON)
				| ((BorderTypeFlags & BORDER_WITHIN)?0:BORDER_FRAME)
				);

	// init close button here.
	AddCaptionButton( pc, NULL, NULL, NULL, 0, NULL );
	//lprintf( "FRAME is %p", pc );
	return pc;
}

//---------------------------------------------------------------------------

PSI_PROC( uintptr_t, GetControlUserData )( PSI_CONTROL pf )
{
	if( pf )
		return pf->psvUser;
	return 0;
}

//---------------------------------------------------------------------------

PSI_PROC( void, SetControlUserData )( PSI_CONTROL pf, uintptr_t psv )
{
	if( pf )
		pf->psvUser = psv;
}

//---------------------------------------------------------------------------
PSI_PROC( int, PSI_AddBindingData )(CTEXTSTR name) {
	// add a binding dta; name may be used lter for diagnostics?
	return 0;
}

PSI_PROC( uintptr_t, PSI_GetBindingData )(PSI_CONTROL pf, int unused)
{
	// use indexer to load per-binding-type value
	if( pf )
		return pf->psvBinding;
	return 0;
}

//---------------------------------------------------------------------------

PSI_PROC( void, PSI_SetBindingData )(PSI_CONTROL pf, int unused, uintptr_t psv)
{
	// use indexer to store per-binding-type value
	if( pf )
		pf->psvBinding = psv;
}


//---------------------------------------------------------------------------

PSI_PROC( void, RevealCommonEx )( PSI_CONTROL pc DBG_PASS )
{
	static int level = 0;
	level++;
	if( pc )
	{
		int parent_hidden = 0;
		int parent_initial = 0;
		int was_hidden = pc->flags.bHidden;
		int revealed = 0;
		PSI_CONTROL parent;
		for( parent = pc?pc->parent:NULL; parent; parent = parent->parent )
		{
			if( parent->flags.bHidden )
				parent_hidden = 1;
			if( parent->flags.bInitial )
				parent_initial = 1;
			//lprintf( "(%p)Parent h %d i %d", parent, parent_hidden, parent_initial );
		}
		//lprintf( "Parent h %d i %d", parent_hidden, parent_initial );
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			_xlprintf(LOG_NOISE DBG_RELAY)( "Revealing %p %s %s"
					, pc
					, pc->pTypeName?pc->pTypeName:"NoTypeName"
					, pc->caption.text?GetText( pc->caption.text ):"" );
#endif
		if( pc->device )
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "showing a renderer..." );
#endif
			revealed = was_hidden;
			pc->flags.bHidden = 0;
			pc->flags.bNoUpdate = 0;
			pc->flags.bRestoring = 1;
			RestoreDisplay( pc->device->pActImg );
			// need to not clear restoring here; it might take a while, (we might have to return before it happens)
		}
		if( was_hidden && ( (level > 1)?1:(pc->flags.bHiddenParent) ) )
		{
			PSI_CONTROL child;
			revealed = 1;
			if( !parent_hidden && !parent_initial )
				InvokeControlRevealed( pc );

			pc->flags.bNoUpdate = 0;
			pc->flags.bHidden = 0;
			if( pc->child )
			{
				//pc->child->flags.bHiddenParent = 0;
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( "Found something hidden... revealing it, and all children." );
#endif
				for( child = pc->child; child; child = child->next )
					if( !child->flags.bHiddenParent )
						RevealCommon( child );
			}
#ifdef DEBUG_UPDAATE_DRAW
			else
				if( g.flags.bLogDebugUpdate )
					lprintf( "Control was hidden, now showing." );
#endif
			//pc->flags.bParentCleaned = 1;
			pc->flags.bHiddenParent = 0;
		}
		if( !pc->flags.bInitial )
		{
			if( !pc->flags.bRestoring )
			{
#ifdef UNDER_CE
				// initial show should already be triggering update draw...
				// this smudge should not happen for device reveals
				//if( !pc->device )
#endif
				if( !g.flags.always_draw )
					SmudgeCommon( pc );
				//UpdateCommon( pc );
			}
		}
#ifdef DEBUG_UPDAATE_DRAW
		else
			if( g.flags.bLogDebugUpdate )
				lprintf( "Initial, no draw" );
#endif
	}
	level--;
}

//---------------------------------------------------------------------------

PSI_PROC( void, DisplayFrameOverOnUnder )( PSI_CONTROL pc, PSI_CONTROL over, PRENDERER pActImg, PSI_CONTROL under )
{
	PPHYSICAL_DEVICE pf = pc?pc->device:NULL;
	if( pc && !pf )
	{
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			lprintf( "-------- OPEN DEVICE" );
#endif
		pc->flags.bInitial = FALSE;
		pc->flags.bOpeningFrameDisplay = TRUE;
		pf = OpenPhysicalDevice( pc, over, pActImg, under );
		pc->flags.bOpeningFrameDisplay = FALSE;
		pc->stack_parent = over;
		pc->stack_child = under;
		if( under )
			under->stack_parent = pc;
		if( over )
			over->stack_child = pc;

#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			lprintf( "-------- device opened" );
#endif
	}
#ifdef DEBUG_CREATE
	lprintf( "-------------------- Display frame has been invoked..." );
#endif
	if( pf )
	{
		// if, by tthe time we show, there's no focus, set that up.
		if( !pf->pFocus )
			FixFrameFocus( pf, FFF_HERE );

		if( pc->flags.bInitial )
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "********* FIrst showing of this window - it's been bINitial for so long..." );
#endif
			// one final draw to make sure everyone is clean.
			//lprintf( "Dipsatch draw." );
			//UpdateCommonEx( pc, FALSE DBG_SRC );
			// clear initial so that further updatecommon's
			// can result in updates to the display.

			// do the move before initial, otherwise certain video
			// drivers may give a redundant paint...

			//MoveSizeDisplay( pf->pActImg, pc->rect.x, pc->rect.y, pc->rect.width, pc->rect.height );

			//Initial prevents all updates in all ways.  So here we're showing, we have a device,
			// and we should be allowed to draw - right? that's what initial is?

			pc->flags.bInitial = FALSE;
			if( g.flags.bLogDebugUpdate )
				lprintf( "inital was set to false...." );
			SetDrawBorder( pc ); // re-sets routines, but also calls draws for border and caption.

			pc->flags.bHidden = TRUE; // fake this.. so reveal shows it... hidden parents will remain hidden
		}
		RevealCommon( pc );
		//lprintf( "Draw common?!" );
		/* wait for the draw event from the video callback... */
		//UpdateCommonEx( pc, FALSE DBG_SRC );
		//SyncRender( pf->pActImg );
		if( pc->flags.bEditSet )
			EditFrame( pc, !pc->flags.bNoEdit );
		pc->flags.auto_opened = 0;
	}

}

PSI_PROC( void, DisplayFrameOverOn )( PSI_CONTROL pc, PSI_CONTROL over, PRENDERER pActImg )
{
	DisplayFrameOverOnUnder( pc, over, pActImg, NULL );
}


//---------------------------------------------------------------------------

PSI_PROC( void, DisplayFrameOver )( PSI_CONTROL pc, PSI_CONTROL over )
{
	//lprintf( "Displayframeover" );
	DisplayFrameOverOn( pc, over, NULL );
	//SmudgeCommon( pc );
}

//---------------------------------------------------------------------------

PSI_PROC( void, DisplayFrameUnder )( PSI_CONTROL pc, PSI_CONTROL under )
{
	//lprintf( "Displayframeover" );
	DisplayFrameOverOnUnder( pc, NULL, NULL, under );
	SmudgeCommon( pc );
}

//---------------------------------------------------------------------------

PSI_PROC( void, DisplayFrame )( PSI_CONTROL pc )
{
	//lprintf( "DisplayFrame" );
	DisplayFrameOverOn( pc, NULL, NULL );
}

//---------------------------------------------------------------------------

PSI_PROC( void, DisplayFrameOn )( PSI_CONTROL pc, PRENDERER pActImg )
{
	//lprintf( "DisplayfFrameOn" );
	DisplayFrameOverOn( pc, NULL, pActImg );
}

//---------------------------------------------------------------------------

PSI_PROC( void, HideControl )( PSI_CONTROL pc )
{
	/* should additionally wrap this with a critical section */
	static int levels;
	int hidden = 0;
	if( !pc )
		return;
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		lprintf( "Hide common %p %s %s"
				, pc
				, pc->pTypeName?pc->pTypeName:"NoTypeName"
				, pc->caption.text?GetText( pc->caption.text ):"" );
#endif
	//PSI_CONTROL _pc = pc;
	levels++;
	if( levels == 1 )
		pc->flags.bHiddenParent = 1;
	if( pc->flags.bHidden ) // already hidden, forget someeone telling me this.
	{
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			lprintf( "Already hidden..." );
#endif
		levels--;
		return;
	}
	pc->flags.bHidden = 1;
	if( pc->flags.bInitial )
	{
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			lprintf( "Control hasn't been shown yet..." );
#endif
		// even if not shown, do mark this hidden control as a hidden parent
		// so it's not auto unhidden on revealing the containing frame.
		levels--;
		return;
	}
	//lprintf( "HIDING %p(%d)", pc, pc->nType );
	if( pc->device )
	{
		//lprintf( "hiding a display image..." );
		pc->flags.bNoUpdate = 1;
		HideDisplay( pc->device->pActImg );
	}
	if( pc )
	{
		PSI_CONTROL child;
		hidden = 1;
		if( GetFrame(pc )->device && GetFrame(pc )->device->pFocus == pc )
			FixFrameFocus( GetFrame(pc)->device, FFF_HERE );
		for( child = pc->child; child; child = child->next )
		{
			/* hide all children, which will trigger /dev/null update */
			HideControl( child );
		}
	}
	if( hidden )
	{
		// tell people that the control is hiding, in case they wanna do additional work
		// the clock for instance disables it's checked entirely when hidden.
		InvokeControlHidden( pc );
	}
	levels--;
	if( !levels && pc )
	{
		pc->flags.bHiddenParent = 1;
		if( pc->parent && hidden )
		{
			//pc->parent->InUse++;
			//lprintf( "Added one to use.." );
			ResetImageBuffers( pc->Window, FALSE );
			if( pc->flags.bTransparent && pc->OriginalSurface )
			{
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( "transparent thing..." );
#endif
				if( !pc->flags.bParentCleaned && pc->OriginalSurface )
				{
#ifdef DEBUG_UPDAATE_DRAW
					if( g.flags.bLogDebugUpdate )
						lprintf( "Restoring old surface..." );
					if( g.flags.bLogDebugUpdate )
						lprintf( "Restoring orignal background... " );
#endif
					BlotImage( pc->Window, pc->OriginalSurface, 0, 0 );
					pc->flags.bParentCleaned = 1;
					{
						PSI_PENDING_RECT upd;
#ifdef DEBUG_UPDAATE_DRAW
						if( g.flags.bLogDebugUpdate )
							lprintf( "pc = %p par = %p", pc, pc->parent );
#endif
						upd.flags.bHasContent = 0;
						upd.flags.bTmpRect = 0;
						//lprintf( "doing update common..." );
#ifdef DEBUG_UPDAATE_DRAW
						if( g.flags.bLogDebugUpdate )
							lprintf( "Flushing this button to the display..." );
#endif
#ifdef BLAT_COLOR_UPDATE_PORTION
						TESTCOLOR=SetAlpha( BASE_COLOR_YELLOW, 0x20 );
#endif
						/*
						DoUpdateFrame( pc
										 , pc->rect.x, pc->rect.y
										 , pc->rect.width, pc->rect.height
										 , TRUE
										  DBG_SRC );
						*/
						DoUpdateFrame( pc->parent
							 , (pc->parent->device?0:pc->parent->rect.x) + pc->parent->surface_rect.x + pc->rect.x
							 , (pc->parent->device?0:pc->parent->rect.y) + pc->parent->surface_rect.y + pc->rect.y
							 , pc->rect.width, pc->rect.height
							 , FALSE
							  DBG_SRC
							 );

					}
				}
				else if( pc->OriginalSurface )
				{
#ifdef DEBUG_UPDAATE_DRAW
					if( g.flags.bLogDebugUpdate )
						lprintf( "Parent was already clean... did nothing" );
#endif
				}
				else
				{
					lprintf( "NOthing to recover?" );
				}
			}
			else
			{
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( "Parent needs to be drawn, no auto recovery available");
#endif
				SmudgeCommon( pc->parent );
			}
		}
#ifdef DEBUG_UPDAATE_DRAW
		else
			if( g.flags.bLogDebugUpdate )
				lprintf( "..." );
#endif
	}
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		lprintf( "levels: %d", levels );
#endif
}

//---------------------------------------------------------------------------

PSI_PROC( void, SizeCommon )( PSI_CONTROL pc, uint32_t width, uint32_t height )
{
	if( pc && pc->Resize )
		pc->Resize( pc, TRUE );
	//if( pc->nType )
	if( pc )
	{
		PPHYSICAL_DEVICE pFrame = GetFrame(pc)->device;
		//ValidatedControlData( PFRAME, CONTROL_FRAME, pFrame, GetFrame( pc ) );
		IMAGE_RECTANGLE old;
		PEDIT_STATE pEditState;
		int32_t delw, delh;

		{
			PFRACTION ix, iy;
			GetCommonScale( pc, &ix, &iy );
			pc->original_rect.width = InverseScaleValue( ix, width );
			pc->original_rect.height = InverseScaleValue( iy, height );
		}
		if( !pc->parent && pc->device && pc->device->pActImg )
		{
			// we now have this accuragely handled.
			//  rect is active (with frame)
			//  oroginal_rect is the size it was before having to extend it
			// somwehere between original_rect changes and rect changes surface_rect is recomputed
			//lprintf( "Enlarging size..." );
			if( pc->BorderType == BORDER_USER_PROC )
			{
				int left, top, right, bottom;
				if( pc->BorderMeasureProc )
				{
					pc->BorderMeasureProc( pc, &left, &top, &right, &bottom );
					width += left + right;
					height += top + bottom;
				}
			}
			else
			{
				width += FrameBorderX(pc, pc->BorderType);
				height += FrameBorderY(pc, pc->BorderType, GetText( pc->caption.text ) );
			}
			SizeDisplay( pc->device->pActImg, width, height );
		}
		delw = (int32_t)width - (int32_t)pc->rect.width;
		delh = (int32_t)height - (int32_t)pc->rect.height;

		old = pc->rect;
		if( pFrame )
			pEditState = &pFrame->EditState;
		else
			pEditState = NULL;

		pc->rect.width = width;
		pc->rect.height = height;

		ResizeImage( pc->Window, width, height );

		if( pEditState && pEditState->flags.bActive &&
			pEditState->pCurrent == pc )
		{
			old.x -= SPOT_SIZE;
			old.y -= SPOT_SIZE;
			old.width += 2*SPOT_SIZE;
			old.height += 2*SPOT_SIZE;
		}

		UpdateSurface( pc );

		if( pFrame && !pFrame->flags.bNoUpdate )
		{
			if( pEditState->flags.bActive &&
				pEditState->pCurrent == pc )
			{
				SetupHotSpots( pEditState );
				SmudgeCommon( pFrame->common );
			}
		}
	}
	if( pc && pc->Resize )
		pc->Resize( pc, FALSE );
}

//---------------------------------------------------------------------------

PSI_PROC( void, SizeCommonRel )( PSI_CONTROL pc, uint32_t w, uint32_t h )
{
	SizeCommon( pc, w + pc->rect.width, h + pc->rect.height );
}

//---------------------------------------------------------------------------

void InvokePosChange( PSI_CONTROL pc, LOGICAL updating )
{
	void (CPROC *OnChanging)(PSI_CONTROL,LOGICAL);
	PCLASSROOT data = NULL;
	PCLASSROOT event_root = GetClassRootEx( pc->class_root, "position_changing" );
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( event_root, &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		OnChanging = GetRegisteredProcedureExx((PCLASSROOT)data,(CTEXTSTR)NULL,void,name,(PSI_CONTROL,LOGICAL));
		if( OnChanging )
		{
			OnChanging( pc, updating );
		}
	}
}

void InvokeMotionChange( PSI_CONTROL pc, LOGICAL updating )
{
	void (CPROC *OnChanging)(PSI_CONTROL,LOGICAL);
	PCLASSROOT data = NULL;
	PCLASSROOT event_root = GetClassRootEx( pc->class_root, "some_parents_position_changing" );
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( event_root, &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		OnChanging = GetRegisteredProcedureExx((PCLASSROOT)data,(CTEXTSTR)NULL,void,name,(PSI_CONTROL,LOGICAL));
		if( OnChanging )
		{
			OnChanging( pc, updating );
		}
	}
}


PSI_PROC( void, MoveCommon )( PSI_CONTROL pc, int32_t x, int32_t y )
{
	if( pc )
	{
		PPHYSICAL_DEVICE pf = pc->device;
		InvokePosChange( pc, TRUE );
		{
			PFRACTION ix, iy;
			GetCommonScale( pc, &ix, &iy );
			pc->original_rect.x = InverseScaleValue( ix, x );
			pc->original_rect.y = InverseScaleValue( iy, y );
		}
		pc->rect.x = x;
		pc->rect.y = y;
		if( pf )
		{
			MoveDisplay( pf->pActImg, x, y );
		}
		else
		{
			IMAGE_RECTANGLE old;
			PPHYSICAL_DEVICE pFrame = GetFrame(pc)->device;
			//ValidatedControlData( PFRAME, CONTROL_FRAME, pFrame, GetFrame( pc ) );
			PEDIT_STATE pEditState;
			old = pc->rect;
			if( pFrame )
			{
				pEditState = &pFrame->EditState;
				if( pEditState->flags.bActive &&
					pEditState->pCurrent == pc )
				{
					old.x -= SPOT_SIZE;
					old.y -= SPOT_SIZE;
					old.width += 2*SPOT_SIZE;
					old.height += 2*SPOT_SIZE;
				}
			}
			else
				pEditState = NULL;
			if( pc->Window )
				MoveImage( pc->Window, x, y );
			if( pEditState&& pEditState->flags.bActive &&
				pEditState->pCurrent == pc )
			{
				SetupHotSpots( pEditState );
				SmudgeCommon( pFrame->common );
			}
		}
		InvokePosChange( pc, FALSE );
	}
}

//---------------------------------------------------------------------------

void ScaleCoords( PSI_CONTROL pc, int32_t* a, int32_t* b )
{
	while( pc && !pc->flags.bScaled )
		pc = pc->parent;
	if( pc )
	{
#ifdef DEBUG_SCALING
		//if( 0 )
		{
			TEXTCHAR tmp[256];
			sLogFraction( tmp, &pc->scalex );
			lprintf( "%p(%s) X is %s", pc, pc->pTypeName, tmp );
			sLogFraction( tmp, &pc->scaley );
			lprintf( "%p(%s) Y is %s", pc, pc->pTypeName, tmp );
		}
#endif
		if( a )
			*a = ScaleValue( &pc->scalex, *a );
		if( b )
			*b = ScaleValue( &pc->scaley, *b );
	}
}

//---------------------------------------------------------------------------

void ApplyRescaleChild( PSI_CONTROL pc, PFRACTION scalex, PFRACTION scaley )
{
	pc = pc->child;
	while( pc )
	{
		//if( 0 )
#ifdef DEBUG_SCALING
		{
			TEXTCHAR tmp[32];
			TEXTCHAR tmp2[32];
			sLogFraction( tmp, scalex );
			sLogFraction( tmp2, scaley );
			lprintf( "updating scaled value %p(%s), X is %s Y is %s", pc, pc->pTypeName, tmp, tmp2 );
		}
#endif
		// if it has no children yet, then resize it according to the new scale...
		if( !(pc->BorderType & BORDER_FIXED) )
		{
			MoveSizeCommon( pc
							  , ScaleValue( scalex, pc->original_rect.x )
							  , ScaleValue( scaley, pc->original_rect.y )
							  , ScaleValue( scalex, pc->original_rect.width )
							  , ScaleValue( scaley, pc->original_rect.height )
							  );
			if( pc && pc->Rescale )
				pc->Rescale( pc );
		}
		// if this has a font, it's children are scaled according to
		// their parent...
		if( !pc->flags.bScaled )
			ApplyRescaleChild( pc, scalex, scaley );
		pc = pc->next;
	}
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void ApplyRescale( PSI_CONTROL pc )
{
	//while( pc && !pc->flags.bScaled )
	//	pc = pc->parent;
	if( pc && pc->flags.bScaled )
	{
		//if( 0 )
#ifdef DEBUG_SCALING
		{
			TEXTCHAR tmp[256];
			lprintf( "Updating scaled value ...." );
			sLogFraction( tmp, &pc->scalex );
			lprintf( "X is %s", tmp );
			sLogFraction( tmp, &pc->scaley );
			lprintf( "Y is %s", tmp );
		}
#endif
		SizeCommon( pc
						  //, ScaleValue( &pc->scalex, pc->original_rect.x )
						  //, ScaleValue( &pc->scaley, pc->original_rect.y )
						  , ScaleValue( &pc->scalex, pc->original_rect.width )
						  , ScaleValue( &pc->scaley, pc->original_rect.height )
						  );
		if( pc && pc->Rescale )
			pc->Rescale( pc );
		ApplyRescaleChild( pc, &pc->scalex, &pc->scaley );
	}
}

//---------------------------------------------------------------------------
void GetCommonScale( PSI_CONTROL pc, PFRACTION *sx, PFRACTION *sy ) {
	while( pc && !pc->flags.bScaled )
		pc = pc->parent;
	if( pc ) {
		( *sx ) = &pc->scalex;
		( *sy ) = &pc->scaley;
	} else {
		static FRACTION one = { 1,1 };
		( *sx ) = &one;
		( *sy ) = &one;
	}
}

//---------------------------------------------------------------------------
void SetCommonScale( PSI_CONTROL pc, PFRACTION sx, PFRACTION sy )
{
	//if( 0 )
#ifdef DEBUG_SCALING
	{
		TEXTCHAR tmp[256];
		lprintf( "Updating scaled value ...." );
		sLogFraction( tmp, &pc->scalex );
		lprintf( "X is %s", tmp );
		sLogFraction( tmp, &pc->scaley );
		lprintf( "Y is %s", tmp );
	}
#endif
	if( sx )
		pc->scalex = *sx;
	if( sy )
		pc->scaley = *sy;
	if( sx || sy )
		pc->flags.bScaled = 1;
	else
		pc->flags.bScaled = 0;
	SetCommonBorder( pc, pc->BorderType );
	if( pc && pc->Rescale )
		pc->Rescale( pc );
	ApplyRescaleChild( pc, sx, sy );
}

//---------------------------------------------------------------------------

static void DispatchFontChange( PSI_CONTROL pc ) {
	while( pc ) {
		if( pc->FontChange )
			pc->FontChange( pc );
		DispatchFontChange( pc->child );
		pc = pc->next;
	}
}

//---------------------------------------------------------------------------

void SetCommonFont( PSI_CONTROL pc, SFTFont font )
{
	if( pc )
	{
		uint32_t w, h;
		uint32_t _w, _h;
#ifdef DEBUG_SCALING
		//if( 0 )
		{
			TEXTCHAR tmp[32];
			TEXTCHAR tmp2[32];
			sLogFraction( tmp, &pc->scalex );
			sLogFraction( tmp2, &pc->scaley );
			lprintf( "Updating scaled value .... %p  %p(%s)  X is %s  Y is %s", font, pc, pc->pTypeName, tmp, tmp2 );
		}
#endif
		// pc->font - prior state before we update it... evyerhting
		// gets rescaled by the new factor...
		// progressive updates will end up with lots of error...
		pc->caption.font = font;
		GetFontRenderData( font, &pc->caption.fontdata, &pc->caption.fontdatalen );

		if( !(pc->BorderType & BORDER_FIXED) )
		{
			GetStringSizeFont( "XXXXX", &_w, &_h, NULL/*pc->caption.font*/ );
			GetStringSizeFont( "XXXXX", &w, &h, font );
			if(h == 0 )
			{
				h = 10;
				//DebugBreak();
			}

			if( w != _w || h != _h )
			{
				//lprintf( "Font is %d/%d,%d/%d", w, _w, h, _h );
				SetFraction( pc->scalex,w,_w);
				SetFraction( pc->scaley,h,_h);
				pc->flags.bScaled = 1;
			}
			else
			{
				SetFraction( pc->scalex,w,_w);
				SetFraction( pc->scaley,h,_h);
				//pc->flags.bScaled = 0;
			}
#ifdef DEBUG_SCALING
			//if( 0 )
			{
				TEXTCHAR tmp[32];
				TEXTCHAR tmp2[32];
				sLogFraction( tmp, &pc->scalex );
				sLogFraction( tmp2, &pc->scaley );
				lprintf( "Updated scaled value .... %p(%s) X is %s  Y is %s", pc, pc->pTypeName, tmp, tmp2 );
			}
#endif
			if( !pc->flags.bInitial && !pc->flags.bNoUpdate )
				SetCommonBorder( pc, pc->BorderType );
			ApplyRescale( pc );
		}
		//else
		//	lprintf( "not setting scaling..." );
		if( !pc->flags.bCreating )
		{
			if( pc->FontChange )
				pc->FontChange( pc );
			DispatchFontChange( pc->child );
		}

		if( !g.flags.always_draw )
		{
			if( !pc->flags.bInitial && !pc->flags.bNoUpdate )
				SmudgeCommon( pc );
		}
	}
}

//---------------------------------------------------------------------------

SFTFont GetCommonFontEx( PSI_CONTROL pc DBG_PASS )
{
	//_xlprintf(1 DBG_RELAY )( "Someone getting font from %p", pc );
	while( pc )
	{
		// lprintf( "Checking control %p for font %p", pc, pc->caption.font );
		if( pc->caption.font )
			return pc->caption.font;
		if( pc->device ) // devices end parent relation also... we maintain (for some reason) parent link to parent control....
			break;
		pc = pc->parent;
	}
	//lprintf( "No control, no font." );
	// results in DefaultFont() when used anyhow...
	return NULL;
}

//---------------------------------------------------------------------------

PSI_PROC( void, MoveCommonRel )( PSI_CONTROL pc, int32_t x, int32_t y )
{
   MoveFrame( pc, x + pc->rect.x, y + pc->rect.y );
}

//---------------------------------------------------------------------------

PSI_PROC( void, MoveSizeCommon )( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t width, uint32_t height )
{
	if( pc )
	{
		PPHYSICAL_DEVICE pf = pc->device;
		PEDIT_STATE pEditState = pf?&pf->EditState:NULL;
		IMAGE_RECTANGLE old;
		// timestamp these...
		//lprintf( "move %p %d,%d %d,%d", pc, x, y, width, height );
		if( !pc )
			return;
		/*
		{
			PSI_CONTROL pf = GetParentControl( pc );
			if( pf && !(pc->BorderType & BORDER_FIXED))
			{
				x = ScaleValue( &pf->scalex, x );
				y = ScaleValue( &pf->scaley, y );
				width = ScaleValue( &pf->scalex, width );
				height = ScaleValue( &pf->scaley, height );
			}
		}
		*/
		if( pf )
		{
			InvokePosChange( pc, TRUE );
			MoveSizeDisplay( pf->pActImg, x, y, width, height );
		}

		// lock out auto updates...
		if( pf )
			pf->flags.bNoUpdate = TRUE;
		old = pc->rect;
		if( pf && pEditState->flags.bActive &&
			pEditState->pCurrent == pc )
		{
			old.x -= SPOT_SIZE;
			old.y -= SPOT_SIZE;
			old.width += 2*SPOT_SIZE;
			old.height += 2*SPOT_SIZE;
		}
		MoveCommon( pc, x, y );
		SizeCommon( pc, width, height );

		if( pf )
		{
			pf->flags.bNoUpdate = FALSE;
		}
		InvokePosChange( pc, FALSE );
	}
}

//---------------------------------------------------------------------------

PSI_PROC( void, MoveSizeCommonRel )( PSI_CONTROL pc, int32_t x, int32_t y, uint32_t width, uint32_t height )
{
    MoveSizeFrame( pc
                     , pc->rect.x + x
                     , pc->rect.y + y
                     , pc->rect.width + width
                     , pc->rect.height + height );
}

//---------------------------------------------------------------------------
#undef GetControl
PSI_PROC( PSI_CONTROL, GetControl )( PSI_CONTROL pContainer, int ID )
{
	PSI_CONTROL pc;
	if( !pContainer )
	   return NULL;
	pc = pContainer->child;
	while( pc )
	{
		if( !pc->flags.bHidden && pc->nID == ID )
			return pc;
		pc = pc->next;
	}
	return pc;
}

//---------------------------------------------------------------------------
#undef GetControl
PSI_PROC( PSI_CONTROL, GetControlByName )( PSI_CONTROL pContainer, const char *ID ) {
	PSI_CONTROL pc;
	if( !pContainer )
		return NULL;
	pc = pContainer->child;
	while( pc ) {
		if( pc->pIDName && ( strcmp( pc->pIDName, ID ) == 0 ) )
			return pc;
		pc = pc->next;
	}
	{
		pc = pContainer->child;
		while( pc ) {
			PSI_CONTROL result = GetControlByName( pc, ID );
			if( result )
				return result;
			pc = pc->next;
		}
	}
	return pc;
}

//---------------------------------------------------------------------------

PSI_PROC( void, EnableCommonUpdates )( PSI_CONTROL common, int bEnable )
{
	if( common )
	{
		while( common->flags.bDirectUpdating )
			Relinquish();
		if( common->flags.bNoUpdate && bEnable )
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "Enable Common Updates on %p", common );
#endif
			common->flags.bNoUpdate = FALSE;
			// probably doing mass updates so just mark the status, and make the application draw.
		}
		else
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( "Disable Common Updates on %p", common );
#endif
			common->flags.bNoUpdate = !bEnable;
		}
	}
}

//---------------------------------------------------------------------------

PSI_PROC( PSI_CONTROL, GetNearControl )( PSI_CONTROL pc, int ID )
{
	PSI_CONTROL parent = pc->parent?pc->parent:pc;
	if( parent->nID == ID ) // parent itself is also a 'near' control
		return parent;
	return GetControl( parent, ID );
}

//---------------------------------------------------------------------------

void GetCommonTextEx( PSI_CONTROL pc, TEXTSTR buffer, int buflen, int bCString )
{
	if( !buffer || !buflen )
		return;
	if( !pc )
	{
		if( buffer && buflen )
			buffer[0] = 0;
		return;
	}
	if( !pc->caption.text )
	{
		buffer[0] = 0;
		return;
	}
	StrCpyEx( buffer, GetText( pc->caption.text ), buflen );
	//lprintf( "GetText was %d", buflen );
	buffer[buflen-1] = 0; // make sure we nul terminate..
	if( bCString ) // use C processing on escapes....
	{
		int n, ofs, escape = 0;
		ofs = 0;
		//lprintf( "GetText was %d", buflen );
		for( n = 0; buffer[n]; n++ )
		{
			if( escape )
			{
				switch( buffer[n] )
				{
				case 'n':
					buffer[n-ofs] = '\n';
					break;
				case 't':
					buffer[n-ofs] = '\t';
					break;
				case '\\':
					buffer[n-ofs] = '\\';
					break;
				default:
					ofs++;
					break;
				}
				escape = FALSE;
				continue;
			}
			if( buffer[n] == '\\' )
			{
				escape = TRUE;
				ofs++;
				continue;
			}
			buffer[n-ofs] = buffer[n];
		}
		buffer[n-ofs] = 0;
	}
}

//---------------------------------------------------------------------------

PSI_PROC( LOGICAL, IsControlHidden )( PSI_CONTROL pc )
{
	PSI_CONTROL parent;
	for( parent = pc; parent; parent = parent->parent )
	{
		if( parent->flags.bNoUpdate || parent->flags.bHidden )
			return TRUE;
	}
	return FALSE;
}

PSI_PROC( Image,  GetControlSurface )( PSI_CONTROL pc )
{
	if( pc )
	{
		return pc->Surface;
	}
	return NULL;
}

//---------------------------------------------------------------------------
PSI_PROC( void, SetControlCaptionImage )( PSI_CONTROL pc, Image image, int pad )
{
	if( pc )
	{
		pc->caption.image = image;
		pc->caption.pad = pad;
		SmudgeCommon( pc );
	}
}

PSI_PROC( void, SetControlText )( PSI_CONTROL pc, CTEXTSTR text )
{
	if( !pc )
		return;
	if( pc->caption.text )
	{
		PTEXT old = pc->caption.text;
		if( text && TextIs( old, text ) )
			return;
		pc->caption.text = NULL;
		LineRelease( old );
	}
	if( text )
	{
		pc->caption.text = SegCreateFromText( text );
	}
	else
		pc->caption.text = NULL;
	if( pc->CaptionChanged )
	{
		//lprintf( "invoke caption changed... " );
		pc->CaptionChanged( pc );
	}
	if( pc->device && pc->device->pActImg )
		SetRendererTitle( pc->device->pActImg, text );
	//else
	// we can use setcontrol text within a smudge condition...  so don't smudge again
	if( !g.flags.always_draw )
	{
		if( !pc->flags.bInitial && !pc->flags.bCleaning )
			SmudgeCommon( pc );
	}
}

//---------------------------------------------------------------------------

void EnableControl( PSI_CONTROL pc, int bEnable )
{
	if( pc )
	{
		pc->flags.bDisable = !bEnable;
		//FixFrameFocus( GetFrame( pc ), FFF_HERE );
		//lprintf( "Control draw %p", pc );
		SmudgeCommon( pc );
	}
}

//---------------------------------------------------------------------------

int IsControlEnabled( PSI_CONTROL pc )
{
	return !pc->flags.bDisable;
}

//---------------------------------------------------------------------------

void LinkInNewControl( PSI_CONTROL parent, PSI_CONTROL elder, PSI_CONTROL child )
{
	PSI_CONTROL pAdd;
	if( parent && child )
	{
		if( elder )
		{
			child->next = elder;
			if( !( child->prior = elder->prior ) )
				parent->child = child;
				elder->prior = child;
		}
		else
		{
			pAdd = parent->child;
			if( pAdd == child )
				return; // already linked.
			while( pAdd && pAdd->next )
			{
				if( pAdd == child )
					return; // already linked.
					//lprintf( "skipping %p...", pAdd );
				pAdd = pAdd->next;
			}
			if( !pAdd )
			{
				//lprintf( "Adding control first..." );
				child->prior = NULL;
				parent->child = child;
			}
			else
			{
				//lprintf( "Adding control after last..." );
				child->prior = pAdd;
				pAdd->next = child;
			}
			child->next = NULL;
		}
		child->parent = parent;
	}
}

//---------------------------------------------------------------------------

void GetControlSize( PSI_CONTROL _pc, uint32_t* w, uint32_t* h )
{
	if( _pc )
	{
		if( w )
			*w = _pc->original_rect.width;
		if( h )
			*h = _pc->original_rect.height;
	}
}

//---------------------------------------------------------------------------

PSI_CONTROL CreateCommonExxx( PSI_CONTROL pContainer
								 // if not typename, must pass type ID...
								, CTEXTSTR pTypeName
								, uint32_t nType
								 // position of control
								, int x, int y
								, int w, int h
								, uint32_t nID
								, CTEXTSTR pIDName // if this is NOT NULL, use Named ID to ID the control.
								// ALL controls have a caption...
							  , CTEXTSTR text
								// fields in this override the defaults...
								// if not 0.
							  , uint32_t ExtraBorderType
								// if this is reloaded...
							  , PTEXT parameters
                        , POINTER extra_param
								//, va_list args
								DBG_PASS )
{
	PSI_CONTROL pResult;
	PROCEDURE proc;
	proc = RealCreateCommonExx( &pResult, pContainer, pTypeName, nType, x, y, w, h, nID, pIDName, text
									  , ExtraBorderType
										// uhmm need to retain this ... as it also means 'private' as in contained
                              // within a control - editing functions should not operate on these either...
										//& ~( BORDER_NO_EXTRA_INIT )  // mask fake border bits to prevent confusion
									  , TRUE DBG_RELAY );
	if( proc )
	{
		if( !((int(CPROC *)(PSI_CONTROL))proc)( pResult ) )
		{
			_xlprintf(1 DBG_RELAY )( "Failed to init the control - destroying it." );
			DestroyCommon( &pResult );
		}
		else
		{
			if( pResult->BorderType & BORDER_CAPTION_CLOSE_BUTTON )
				AddCaptionButton( pResult, NULL, NULL, NULL, 0, NULL );
		}
	}
	pResult->flags.bCreating = 0;
	if( pResult ) // no point in doing anything extra if the initial init fails.
	{
		TEXTCHAR mydef[256];
		if( pTypeName )
			tnprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY "/control/%s/rtti/extra init", pTypeName );
		else
			tnprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY "/control/%" _32f "/rtti/extra init", nType );
		if( !(ExtraBorderType & BORDER_NO_EXTRA_INIT ) )
		{
			int (CPROC *CustomInit)(PSI_CONTROL);
			CTEXTSTR name;
			PCLASSROOT data = NULL;
			pResult->flags.private_control = 0;
			{
				int (CPROC *CustomInit)(PSI_CONTROL);
				// dispatch for a common proc that is registered to handle extra init for
				// any control...
				for( name = GetFirstRegisteredName( "psi/control/rtti/extra init", &data );
						name;
						name = GetNextRegisteredName( &data ) )
				{
					CustomInit = GetRegisteredProcedureExx((PCLASSROOT)data,(CTEXTSTR)NULL,int,name,(PSI_CONTROL));
					if( CustomInit )
					{
						if( !CustomInit( pResult ) )
						{
							lprintf( "extra init has returned failure... so what?" );
						}
					}
				}
			}
			// then here lookup the specific control type's extra init proc...
			for( name = GetFirstRegisteredName( mydef, &data );
					name;
					name = GetNextRegisteredName( &data ) )
			{
				CustomInit = GetRegisteredProcedureExx((PCLASSROOT)data,(CTEXTSTR)NULL,int,name,(PSI_CONTROL));
				if( CustomInit )
				{
					if( !CustomInit( pResult ) )
					{
						lprintf( "extra init has returned failure... so what?" );
					}
				}
			}
		}
		else
		{
			pResult->flags.private_control = 1;
		}
	}
	if( pContainer && pContainer->AddedControl )
		pContainer->AddedControl( pContainer, pResult );
	if( pContainer )
	{
		// creation could have caused it to be hidden...
		pResult->flags.bInitial = pContainer->flags.bInitial;
		if( !pResult->flags.bHiddenParent && !pResult->flags.bInitial )
		{
			pResult->flags.bHidden = pContainer->flags.bHidden;
			// same thing as DeleteUse would do
			UpdateCommon( pResult );
		}
	}
	return pResult;

}

PSI_CONTROL MakeControl( PSI_CONTROL pFrame
					, uint32_t nType
					, int x, int y
					, int w, int h
					, uint32_t nID
					//, ...
					)
{
	return CreateCommonExx( pFrame, NULL, nType, x, y, w, h, nID, NULL, 0, NULL, NULL DBG_SRC );
}

PSI_CONTROL MakeControlParam( PSI_CONTROL pFrame
								, uint32_t nType
								, int x, int y
								, int w, int h
								, uint32_t nID
								, POINTER parameter
								)
{
	return CreateCommonExx( pFrame, NULL, nType, x, y, w, h, nID, NULL, 0, NULL, parameter DBG_SRC );
}

PSI_CONTROL MakePrivateControl( PSI_CONTROL pFrame
								  , uint32_t nType
								  , int x, int y
								  , int w, int h
								  , uint32_t nID
								  )
{
	return CreateCommonExx( pFrame, NULL, nType
								 , x, y
								 , w, h
								 , nID, NULL
								 , BORDER_NO_EXTRA_INIT|BORDER_FIXED
								 , NULL, NULL DBG_SRC );
}

PSI_CONTROL MakePrivateNamedControl( PSI_CONTROL pFrame
								  , CTEXTSTR pType
								  , int x, int y
								  , int w, int h
								  , uint32_t nID
								   )
{
	return CreateCommonExx( pFrame, pType, 0
								 , x, y
								 , w, h
								 , nID, NULL
								 , BORDER_NO_EXTRA_INIT
								 , NULL, NULL DBG_SRC );
}

PSI_CONTROL MakeCaptionedControl( PSI_CONTROL pFrame
									 , uint32_t nType
									 , int x, int y
									 , int w, int h
									 , uint32_t nID
									 , CTEXTSTR caption
									 //, ...
									 )
{
	return CreateCommonExx( pFrame, NULL, nType, x, y, w, h, nID, caption, 0, NULL, NULL DBG_SRC );
}

PSI_PROC( PSI_CONTROL, MakeNamedCaptionedControlByName )( PSI_CONTROL pContainer
																		  , CTEXTSTR pType
																		  , int x, int y
																		  , int w, int h
																		  , CTEXTSTR pIDName
																		  , uint32_t nID
																		  , CTEXTSTR caption
																		  )
{
	return CreateCommonExxx( pContainer, pType, 0, x, y, w, h, nID, pIDName, caption, 0, NULL, NULL DBG_SRC );
}

PSI_PROC( PSI_CONTROL, MakeNamedCaptionedControl )( PSI_CONTROL pContainer
															 , CTEXTSTR pType
															 , int x, int y
															 , int w, int h
															 , uint32_t nID
															 , CTEXTSTR caption
															 )
{
	return CreateCommonExx( pContainer, pType, 0, x, y, w, h, nID, caption, 0, NULL, NULL DBG_SRC );
}


PSI_CONTROL VMakeCaptionedControl( PSI_CONTROL pFrame
									 , uint32_t nType
									 , int x, int y
									 , int w, int h
									 , uint32_t nID
									 , CTEXTSTR caption
									  //, va_list args
									  )
{
	return CreateCommonExx( pFrame, NULL, nType, x, y, w, h, nID, caption, 0, NULL, NULL DBG_SRC );
}


PSI_CONTROL MakeNamedControl( PSI_CONTROL pFrame
                    , CTEXTSTR pType
						  , int x, int y
						  , int w, int h
						  , uint32_t nID
								//, ...
								)
{
	//va_list args;
   //va_start( args, nID );
   return CreateCommonExx( pFrame, pType, 0, x, y, w, h, nID, NULL, 0, NULL, NULL DBG_SRC );
}

PSI_CONTROL VMakeControl( PSI_CONTROL pFrame
                        , uint32_t nType
                        , int x, int y
                        , int w, int h
                        , uint32_t nID
                        )
{
   return CreateCommonExx( pFrame, NULL, nType, x, y, w, h, nID, NULL, 0, NULL, NULL DBG_SRC );
}

PSI_CONTROL RestoreControl( PSI_CONTROL pFrame
                          , uint32_t nType
                          , int x, int y
                          , int w, int h
                          , uint32_t nID
                          , PTEXT parameters )
{
   return CreateCommonExx( pFrame, NULL, nType, x, y, w, h, nID, NULL, 0, parameters, NULL DBG_SRC );
}

//---------------------------------------------------------------------------

void DestroyCommonExx( PSI_CONTROL *ppc, int level DBG_PASS )
{
	PSI_CONTROL pNext;
	if( ppc && *ppc )
	{
		PSI_CONTROL pc = *ppc;
		// need to get what frame this control is in before unlinking it from the frame!
		PSI_CONTROL pFrame = GetFrame( pc );
		if( pFrame )
		{
			if( pFrame->stack_parent )
				pFrame->stack_parent->stack_child = pFrame->stack_child;
			if( pFrame->stack_child )
				pFrame->stack_child->stack_parent = pFrame->stack_parent;
			pFrame->stack_parent = NULL;
			pFrame->stack_child = NULL;
		}
		if( !pc->flags.bDestroy )
		{
			if( pc->device )
			{
				AddUse( pc );
				DetachFrameFromRenderer( pc );
				DeleteUse( pc );
			}
			pc->flags.bDestroy = 1;
		}
		if( ( pNext = pc->next ) != NULL )
		{
			pc->next->prior = pc->prior;
			pc->next = NULL;
		}
		if( pc->prior )
		{
			pc->prior->next = pNext;
			pc->prior = NULL;
		}
		else if( pc->parent )
		{
			if( pc->parent->device )
				if( pc->parent->device->pFocus == pc )
				{
					pc->parent->device->pFocus = NULL;
					if( !pc->parent->flags.bDestroy )
						FixFrameFocus( pc->parent->device, FFF_FORWARD );
				}
			if( pc->parent->child == pc )
				pc->parent->child = pNext;
			if( !pc->parent->flags.bDestroy )
				SmudgeCommon( pc->parent );
			pc->parent = NULL;
		}

		if( !ChildInUse( pc, 0 ) && !pc->InWait )
		{
			level++;
			AddUse( pc );
			//lprintf( "Destroying control %p", pc );
			while( pc->child )
			{
				//lprintf( "destroying child control %p", pc->child );
				DestroyCommonExx( &pc->child, level DBG_RELAY );
			}
			if( pc->caption.text )
			{
				//lprintf( "Release caption text" );
				LineReleaseEx( pc->caption.text DBG_RELAY );
				pc->caption.text = NULL;
			}
			{
				if( pc->Destroy )
					pc->Destroy( pc );
				{
					int (CPROC *CustomDestroy)(PSI_CONTROL);
					TEXTCHAR mydef[256];
					CTEXTSTR name;
					PCLASSROOT data = NULL;
					tnprintf( mydef, sizeof( mydef ), "psi/control/rtti/extra destroy" );
					for( name = GetFirstRegisteredName( mydef, &data );
						 name;
						  name = GetNextRegisteredName( &data ) )
					{
						CustomDestroy = GetRegisteredProcedureExx((PCLASSROOT)data,(CTEXTSTR)NULL,int,name,(PSI_CONTROL));
						if( CustomDestroy )
						{
							//lprintf( "Invoking custom destroy" );
							if( !CustomDestroy( pc ) )
							{
								//lprintf( "extra destroy has returned failure... so what?" );
							}
						}
					}

					tnprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY "/control/%d/rtti/extra destroy", pc->nType );
					for( name = GetFirstRegisteredName( mydef, &data );
						 name;
						  name = GetNextRegisteredName( &data ) )
					{
						CustomDestroy = GetRegisteredProcedureExx((PCLASSROOT)data,(CTEXTSTR)NULL,int,name,(PSI_CONTROL));
						if( CustomDestroy )
						{
							if( !CustomDestroy( pc ) )
							{
								//lprintf( "extra destroy has returned failure... so what?" );
							}
						}
					}
				}
				Release( ControlData( POINTER, pc ) );
				pc->pUser = NULL;
			}
			if( !pc->device )
			{
				// this may not be destroyed if it's
				// the main frame, and the image is that
				// of the renderer...
				UnmakeImageFile( pc->Window );
			}
			if( pc->Surface )
				UnmakeImageFile( pc->Surface );
			if( pc->pTypeName )
			{
				//Release( pc->pTypeName );
				pc->pTypeName = NULL;
			}
			if( pc->pIDName )
			{
				Release( (POINTER)pc->pIDName );
				pc->pIDName = NULL;
			}
			{
				// get frame results null if it's being destroyed itself.
				// and I need to always clear this if it's able at all to be done.
				//GetFrame( pc );
				if( pFrame )
				{
					PPHYSICAL_DEVICE pf = pFrame->device;
					//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, GetFrame( pc ) );
					if( pf )
					{
						if( pf->pCurrent == pc )
						{
							pf->pCurrent = NULL;
						}
						//else
						//   lprintf( "Current is not %p it is %p", pc, pf->pCurrent );
						if( pf->pFocus == pc )
						{
							pf->pFocus = NULL;
							FixFrameFocus( pf, FFF_FORWARD );
						}
						if( pf->EditState.flags.bActive && pf->EditState.pCurrent == pc )
						{
							pf->EditState.flags.bHotSpotsActive = 0;
							pf->EditState.pCurrent = NULL;
							{
								IMAGE_RECTANGLE upd = pf->EditState.bound;
								upd.x -= SPOT_SIZE;
								upd.y -= SPOT_SIZE;
								lprintf( "update some controls is a edit thing..." );
								SmudgeSomeControls( pf->common, &upd );
							}
						}
					}
					//else
					//   lprintf( "no device which might have a current..." );
				}
				//else
				//   lprintf( "no frame to unmake current" );
			}
			if( pc->device ) // only thing which may have a commonwait
			{
				PPSI_COMMON_BUTTON_DATA pcbd = &pc->parent->pCommonButtonData;
				if( pcbd )
				{
					if( pcbd->event )
						pcbd->event( pcbd->psv, pc, *pcbd->done_value, *pcbd->okay_value );
					else
						WakeThread( pcbd->thread );
				}
			}
			Release( pc->_DrawThySelf );
			Release( pc->_DrawDecorations );
			Release( pc->_MouseMethod );
			Release( pc->_KeyProc );
			if( pc->OriginalSurface )
				UnmakeImageFile( pc->OriginalSurface );
			Release( pc );
		}
		//UnmakeImageFileEx( pf->Surface DBG_RELAY );
		*ppc = pNext;
		level--;
		if( !level )
			*ppc = NULL;
	}

}

//---------------------------------------------------------------------------

void DestroyCommonEx( PSI_CONTROL *ppc DBG_PASS )
{
	DestroyCommonExx( ppc, 0 DBG_RELAY );
}

//---------------------------------------------------------------------------

PSI_PROC( int, GetControlID )( PSI_CONTROL pc )
{
	if( pc )
		return pc->nID;
	return -1;
}

//---------------------------------------------------------------------------

PSI_PROC( void, SetControlID )( PSI_CONTROL pc, int ID )
{
	if( pc )
	{
		pc->nID = ID;
		if( pc->pIDName )
			Release( (POINTER)pc->pIDName );
		pc->pIDName = GetResourceIDName( pc->pTypeName, ID );
	}
}
//---------------------------------------------------------------------------

PSI_PROC( void, SetControlIDName )( PSI_CONTROL pc, TEXTCHAR *IDName )
{
	if( pc )
	{
		// no ID to default to, so pass -1
		pc->nID = GetResourceID( pc->parent, IDName, -1 );
	}
}
//------------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------


static void CPROC ButtonOkay( uintptr_t psv, PSI_CONTROL pc )
{
	PPSI_COMMON_BUTTON_DATA pcbd = pc->parent?&pc->parent->pCommonButtonData:NULL;
	if( pcbd ) {
		{
			int *val = (int*)psv;
			if( val )
				*val = TRUE;
			else
				if( pcbd->done_value )
					( *pcbd->done_value ) = TRUE;
		}
		if( pcbd->event )
			pcbd->event( pcbd->psv, pc, pcbd->done_value?*pcbd->done_value:0, pcbd->okay_value?*pcbd->okay_value:0 );
		else
			if( pcbd->thread )
				WakeThread( pcbd->thread );
	}
}


//---------------------------------------------------------------------------

PSI_PROC( void, InitCommonButton )( PSI_CONTROL pc, int *value )
{
   //ConfigButton( pc, NULL, ButtonOkay, (uintptr_t)value );
}

//---------------------------------------------------------------------------

void SetCommonButtons( PSI_CONTROL pf
                     , int *pdone
                     , int *pokay )
{
	if( pf )
	{
		PPSI_COMMON_BUTTON_DATA pcbd;
		SetButtonPushMethod( GetControl( pf, BTN_ABORT ), ButtonOkay, (uintptr_t)NULL );
		pcbd = &pf->pCommonButtonData;
		if( !pcbd->okay_value ) {
			PSI_CONTROL pc = GetControl( pf, BTN_OKAY );
			if( pc ) {
				SetButtonPushMethod( pc, ButtonOkay, (uintptr_t)pokay );
				pcbd->okay_value = pokay;
			}
		}
		if( !pcbd->done_value ) {
			PSI_CONTROL pc = GetControl( pf, BTN_CANCEL );
			if( pc ) {
				SetButtonPushMethod( pc, ButtonOkay, (uintptr_t)pdone );
				pcbd->done_value = pdone;
			}
		}
	}
}

void AddCommonButtonsEx( PSI_CONTROL pf
                       , int *done, CTEXTSTR donetext
                       , int *okay, CTEXTSTR okaytext )
{
	if( pf )
	{
		// scaled!
		int w = pf->rect.width;//FrameBorderX( pf->BorderType );
		int h = pf->rect.height;//FrameBorderY( pf, pf->BorderType, NULL );
		PPSI_COMMON_BUTTON_DATA pcbd;
		PSI_CONTROL pc;
		int x, x2;
		int y;
		//  lprintf( "Buttons will be added at... %d, %d"
		//  		 , w //pf->surface_rect.width - FrameBorderX( pf->BorderType )
		//  		 , h //pf->surface_rect.height - FrameBorderY( pf, pf->BorderType, NULL )
		// 		 );
		if( done && okay )
		{
			x = w - ( 2*COMMON_BUTTON_PAD + ScaleValue( &pf->scalex, (COMMON_BUTTON_WIDTH+COMMON_BUTTON_WIDTH) ) );
			x2 = x + ( ScaleValue( &pf->scalex, COMMON_BUTTON_WIDTH  ) + COMMON_BUTTON_PAD );
		}
		else if( (done&&donetext) || (okay&& okaytext) )
		{
			x = w - ( ScaleValue( &pf->scalex, (COMMON_BUTTON_WIDTH) ) + COMMON_BUTTON_PAD );
			x2 = x;
		}
		else
			return;
		y = h - ( COMMON_BUTTON_PAD + ScaleValue( &pf->scaley, COMMON_BUTTON_HEIGHT ) );
		x = InverseScaleValue( &pf->scalex, x );
		x2 = InverseScaleValue( &pf->scalex, x2 );
		y = InverseScaleValue( &pf->scaley, y );
		pcbd = &pf->pCommonButtonData;
		pcbd->okay_value = okay;
		pcbd->done_value = done;
		pcbd->thread = 0;

		if( okay && okaytext )
		{
			pc = MakeButton( pf
								, x, y
								, COMMON_BUTTON_WIDTH, COMMON_BUTTON_HEIGHT
								, BTN_OKAY, okaytext, 0, ButtonOkay, (uintptr_t)okay );
			//SetCommonUserData( pc, (uintptr_t)pcbd );
		}
		if( done && donetext )
		{
			pc = MakeButton( pf
						  , x2, y
						  , COMMON_BUTTON_WIDTH, COMMON_BUTTON_HEIGHT
						  , BTN_CANCEL, donetext, 0, ButtonOkay, (uintptr_t)done );
			//SetCommonUserData( pc, (uintptr_t)pcbd );
		}
	}
}

//---------------------------------------------------------------------------

void AddCommonButtons( PSI_CONTROL pf, int *done, int *okay )
{
	AddCommonButtonsEx( pf, done, "Cancel", okay, "OK" );
}

//---------------------------------------------------------------------------

_MOUSE_NAMESPACE
PSI_PROC( int, InvokeDefaultButton )( PSI_CONTROL pcNear, int bCancel )
{
	PSI_CONTROL pcf = GetFrame( pcNear );
	if( pcf )
	{
		PPHYSICAL_DEVICE pf = pcf->device;
		//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, pcf );
		PSI_CONTROL pc;
		if( !pf )
			return 0;

		if( bCancel )
			pc = GetControl( pcf, pf->nIDDefaultCancel );
		else
			pc = GetControl( pcf, pf->nIDDefaultOK );
		if( pc )
		{
			//extern void InvokeButton( PSI_CONTROL );
			InvokeButton( pc );
			return 1;
		}
	}
	return 0;
}

int InvokeDefault( PSI_CONTROL pc, int type )
{
	return InvokeDefaultButton( pc, type );
}
_MOUSE_NAMESPACE_END
//---------------------------------------------------------------------------

void SetNoFocus( PSI_CONTROL pc )
{
	if( pc )
		pc->flags.bNoFocus = TRUE;
}

//---------------------------------------------------------------------------

void *ControlExtraData( PSI_CONTROL pc )
{
	return (void*)(pc+1);
}

//---------------------------------------------------------------------------
#undef GetFrame
// get top level frame... the root of all frames.
PSI_PROC( PSI_CONTROL, GetFrame )( PSI_CONTROL pc )
//#define GetFrame(c) GetFrame((PSI_CONTROL)c)
{
	while( pc )
	{
		if( ( !pc->parent || (pc->device) || (!pc->Window->pParent) ) && !pc->flags.bDestroy )
			return pc;
		pc = pc->parent;
	}
	return NULL;
}

//---------------------------------------------------------------------------

#undef GetCommonParent
// this should ge depricated; for bad syntax
PSI_CONTROL GetCommonParent( PSI_CONTROL pc )
{
	if( pc )
		return pc->parent;
	return NULL;
}

//---------------------------------------------------------------------------

PSI_CONTROL GetParentControl( PSI_CONTROL pc )
{
	if( pc )
		return pc->parent;
	return NULL;
}

//---------------------------------------------------------------------------
PSI_CONTROL GetFirstChildControl( PSI_CONTROL pc )
{
	if( pc )
		return pc->child;
	return NULL;
}

//---------------------------------------------------------------------------
PSI_CONTROL GetNextControl( PSI_CONTROL pc )
{
	if( pc )
		return pc->next;
	return NULL;
}

//---------------------------------------------------------------------------
PSI_PROC( void, ProcessControlMessages )( void )
{
	Idle();
}

//---------------------------------------------------------------------------
#undef CommonLoop
PSI_PROC( void, CommonLoop )( int *done, int *okay )
{
	lprintf( "COMMON LOOP IS DEPRECATED, PLEASE UPDATE TO ASYNC METHODS" );
	PPSI_COMMON_BUTTON_DATA pcbd;
	//AddWait( pc );
	pcbd = New( PSI_COMMON_BUTTON_DATA );
	pcbd->okay_value = okay;
	pcbd->done_value = done;
	pcbd->thread = MakeThread();
	pcbd->flags.bWaitOnEdit = 0;
	while( //!pc->flags.bDestroy
			!( ( pcbd->okay_value )?( *pcbd->okay_value ):0 )
			&& !( ( pcbd->done_value )?( *pcbd->done_value ):0 )
		  )
		if( !Idle() )
		{
			// this is a legtitimate condition, that does not fail.
			//lprintf( "Sleeping forever, cause I'm not doing anything else..." );
			WakeableSleep( SLEEP_FOREVER );
		}
	pcbd->thread = NULL;
}

//---------------------------------------------------------------------------
PSI_PROC( void, CommonWaitEndEdit)( PSI_CONTROL *pf ) // a frame in edit mode, once edit mode done, continue
{
	lprintf( "COMMON LOOP IS DEPRECATED, PLEASE UPDATE TO ASYNC METHODS" );
	PPSI_COMMON_BUTTON_DATA pcbd;
	AddWait( (*pf) );
	pcbd = &(*pf)->pCommonButtonData;
	pcbd->thread = MakeThread();
	pcbd->flags.bWaitOnEdit = 1;
	while( !(*pf)->flags.bDestroy
			&& ( pcbd->flags.bWaitOnEdit )
		  )
		if( !Idle() )
		{
			//lprintf( "Sleeping forever, cause I'm not doing anything else..>" );
			WakeableSleep( SLEEP_FOREVER );
		}
	pcbd->thread = NULL;
	DeleteWaitEx( pf DBG_SRC );
}


PSI_PROC( void, PSI_HandleStatusEvent )( PSI_CONTROL pc, void (*f)( uintptr_t psv, PSI_CONTROL pc, int done, int okay ), uintptr_t psv ) { // perhaps give a callback for within the loop?

	PPSI_COMMON_BUTTON_DATA pcbd;
	AddWait( pc );
	pcbd = &pc->pCommonButtonData;
	pcbd->thread = MakeThread();
	pcbd->flags.bWaitOnEdit = 0;
	pcbd->event = f;
   pcbd->psv = psv;
	pcbd->thread = NULL;
}

PSI_PROC( void, CommonWait)( PSI_CONTROL pc ) // perhaps give a callback for within the loop?
{
	lprintf( "COMMON LOOP IS DEPRECATED, PLEASE UPDATE TO ASYNC METHODS" );
	if( pc )
	{
		PPSI_COMMON_BUTTON_DATA pcbd;
		AddWait( pc );
		pcbd = &pc->pCommonButtonData;
		pcbd->thread = MakeThread();
		pcbd->flags.bWaitOnEdit = 0;
		while( !pc->flags.bDestroy
				&& !( ( pcbd->okay_value )?( *pcbd->okay_value ):0 )
				&& !( ( pcbd->done_value )?( *pcbd->done_value ):0 )
			  )
		{
			//lprintf( "not done..." );
			if( !Idle() )
			{
				//lprintf( "Sleeping forever, cause I'm not doing anything else..." );
				WakeableSleep( SLEEP_FOREVER );
			}
			else
			{
				// is thread, did a relinq... but it's UI so we can sleep a little
				WakeableSleep( 25 );
			}
		}
		pcbd->thread = NULL;
		DeleteWait( pc );
	}
}

//---------------------------------------------------------------------------

PSI_PROC( void, OrphanCommonEx )( PSI_CONTROL pc, LOGICAL bDraw )
{
	// Removes the control from relation with it's parent...
	PSI_CONTROL pParent;

	if( !pc || !pc->parent )
		return;
	{
		PPHYSICAL_DEVICE pf = GetFrame(pc)->device;
		//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, GetFrame( pc ) );
		if( pf )
		{
			pf->flags.bCurrentOwns = FALSE;
			pf->pCurrent = NULL;
		}
	}
	pParent = pc->parent;
	if( pc->next )
		pc->next->prior = pc->prior;
	if( pc->prior )
		pc->prior->next = pc->next;
	else
		if( pc->parent )
			pc->parent->child = pc->next;
	pc->parent = NULL;
	pc->next = NULL;
	pc->prior = NULL;
	OrphanSubImage( pc->Window );
	if( bDraw )
	{
		UpdateCommonEx( pParent, bDraw DBG_SRC ); // and of course all sub controls
	}
	else
	{
		// tell the parent that we're needing an update here...
		//SmudgeSomeControls( pParent, (IMAGE_RECTANGLE*)pc->Window );
	}
}

PSI_PROC( void, OrphanCommon )( PSI_CONTROL pc )
{
	OrphanCommonEx( pc, FALSE );
}
//---------------------------------------------------------------------------

PSI_PROC( void, AdoptCommonEx )( PSI_CONTROL pFoster, PSI_CONTROL pElder, PSI_CONTROL pOrphan, LOGICAL bDraw )
{
	// Puts a control under control of a new parent...
	if( ( !pFoster || !pOrphan
	    || pOrphan->parent // has a parent...
	    ) && !pOrphan->device // might have been seperated cause of DetachChildFrames
	  ) // is a master level frame - no good.
	{
		//lprintf( "Failing adopt: %p %p %p %d"
		//       , pFoster, pOrphan
		//       , pOrphan?pOrphan->parent:NULL
		//       , pOrphan?pOrphan->nType:-1 );
		if( !pOrphan->device )
			return;
	}
	// foster has adopted a child...
	// and therefore this child cannot be saved (directly)
	// areas that this applies to include Sheet controls.
	// sheets are saved in separate files from the parent frame.
	if( pOrphan->device )
		DetachFrameFromRenderer( pOrphan );
	pFoster->flags.bAdoptedChild = 1;// adopted children are saved as subsections of XML(?) sub files(?)
	if( !pOrphan->parent )
		LinkInNewControl( pFoster, pElder, pOrphan );
	if( pOrphan->Window )
	{
		if( pOrphan->Surface )
			OrphanSubImage( pOrphan->Surface );
		UnmakeImageFile( pOrphan->Window );
		pOrphan->Window = NULL;
	}
	if( !pOrphan->Window )
	{
		// its original-rect might have been messed up - so fall back to detached which was saved
		if( pOrphan->flags.detached )
			pOrphan->Window = MakeSubImage( pFoster->Surface
				, pOrphan->detached_at.x, pOrphan->detached_at.y
				, pOrphan->detached_at.width, pOrphan->detached_at.height
				);
		else
			pOrphan->Window = MakeSubImage( pFoster->Surface
				, ScaleValue( &pOrphan->scalex, pOrphan->original_rect.x )
				, ScaleValue( &pOrphan->scaley, pOrphan->original_rect.y )
				, ScaleValue( &pOrphan->scalex, pOrphan->original_rect.width )
				, ScaleValue( &pOrphan->scaley, pOrphan->original_rect.height )
			);
	}
	if( pOrphan->Surface )
		AdoptSubImage( pOrphan->Window, pOrphan->Surface );
	else
	{
		lprintf( "!!!!!!!! No Surface on control!?" );
	}
	ApplyRescale( pOrphan );
	if( !g.flags.always_draw )
		if( bDraw )
			UpdateCommonEx( pFoster, bDraw DBG_SRC ); // and of course all sub controls
}

//---------------------------------------------------------------------------

PSI_PROC( void, AdoptCommon )( PSI_CONTROL pFoster, PSI_CONTROL pElder, PSI_CONTROL pOrphan )
{
	AdoptCommonEx( pFoster, pElder, pOrphan, TRUE );
}

//---------------------------------------------------------------------------

PSI_CONTROL CreateControlExx( PSI_CONTROL pFrame
								  , uint32_t attr
								  , int x, int y
								  , int w, int h
								  , int nID
								  , int BorderType
								  , int extra
								  , ControlInitProc InitProc
								  , uintptr_t psvInit
									DBG_PASS )
{
	return NULL;
}

//---------------------------------------------------------------------------

void DestroyFrameEx( PSI_CONTROL pc DBG_PASS )
{
	DestroyCommon( &pc );
}

//---------------------------------------------------------------------------

void DestroyControlEx(PSI_CONTROL pc DBG_PASS )
{
	DestroyCommon( &pc );
}

//---------------------------------------------------------------------------

void GetPhysicalCoordinate( PSI_CONTROL relative_to, int32_t *_x, int32_t *_y, int include_surface )
{
	int32_t x = (*_x);
	int32_t y = (*_y);
	int32_t wx, wy;
	PSI_CONTROL frame = GetFrame( relative_to );
	if( frame->device && frame->device->pActImg )
		GetDisplayPosition( frame->device->pActImg, &wx, &wy, NULL, NULL );
	else
	{
		wx = 0;
		wy = 0;
	}
	x += wx;
	y += wy;
	while( relative_to )
	{
		if( include_surface )
			x += relative_to->surface_rect.x;
		if( relative_to->parent )
			x += relative_to->rect.x;
		if( include_surface )
			y += relative_to->surface_rect.y;
		if( relative_to->parent )
			y += relative_to->rect.y;
		relative_to = relative_to->parent;
		include_surface = 1;
	}
	(*_x) = x;
	(*_y) = y;
}

PRENDERER GetFrameRenderer( PSI_CONTROL pcf )
{
	if( pcf )
	{
		PPHYSICAL_DEVICE pf = GetFrame( pcf )->device;
		//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, pcf );
		if( pf )
			return pf->pActImg;
	}
	return NULL;
}

//---------------------------------------------------------------------------

PSI_CONTROL GetFrameFromRenderer( PRENDERER renderer )
{
	INDEX idx;
	PSI_CONTROL pc;
	LIST_FORALL( g.shown_frames, idx, PSI_CONTROL, pc )
	{
		if( pc->device && pc->device->pActImg == renderer )
			break;
	}
	return pc;
}

//---------------------------------------------------------------------------

void AddCommonDraw( PSI_CONTROL pc
						, int (CPROC*Draw)( PSI_CONTROL pc ) )
{
	if( Draw )
	{
		pc->_DrawThySelf = (__DrawThySelf*)Reallocate( pc->_DrawThySelf, ( pc->n_DrawThySelf + 1 ) * sizeof( pc->_DrawThySelf[0] ) );
		pc->_DrawThySelf[pc->n_DrawThySelf++] = Draw;
	}

}

//---------------------------------------------------------------------------

void SetCommonDraw( PSI_CONTROL pc
									  , int (CPROC*Draw)( PSI_CONTROL pc ) )
{
	if( Draw )
	{
		pc->_DrawThySelf = (__DrawThySelf*)Preallocate( pc->_DrawThySelf, ( pc->n_DrawThySelf + 1 ) * sizeof( pc->_DrawThySelf[0] ) );
		pc->_DrawThySelf[0] = Draw;
		pc->n_DrawThySelf++;
	}
}
//---------------------------------------------------------------------------

void SetCommonDrawDecorations( PSI_CONTROL pc
									  , __DrawDecorations DrawDecorations )
{
	if( DrawDecorations )
	{
		pc->_DrawDecorations = (__DrawDecorations*)Preallocate( pc->_DrawDecorations, ( pc->n_DrawDecorations + 1 ) * sizeof( pc->_DrawDecorations[0] ) );
		pc->_DrawDecorations[0] = DrawDecorations;
		pc->n_DrawDecorations++;
	}
}

//---------------------------------------------------------------------------

void AddCommonMouse( PSI_CONTROL pc
									  , int (CPROC*MouseMethod)(PSI_CONTROL, int32_t x, int32_t y, uint32_t b ) )
{
	if( MouseMethod )
	{
		pc->_MouseMethod = (__MouseMethod*)Reallocate( pc->_MouseMethod, ( pc->n_MouseMethod + 1 ) * sizeof( pc->_MouseMethod[0] ) );
		pc->_MouseMethod[pc->n_MouseMethod++] = MouseMethod;
	}
}

//---------------------------------------------------------------------------

void SetCommonMouse( PSI_CONTROL pc
									  , int (CPROC*MouseMethod)(PSI_CONTROL, int32_t x, int32_t y, uint32_t b ) )
{
	if( MouseMethod )
	{
		pc->_MouseMethod = (__MouseMethod*)Preallocate( pc->_MouseMethod, ( pc->n_MouseMethod + 1 ) * sizeof( pc->_MouseMethod[0] ) );
		pc->_MouseMethod[0] = MouseMethod;
		pc->n_MouseMethod++;
	}
}

//---------------------------------------------------------------------------

void AddCommonAcceptDroppedFiles( PSI_CONTROL pc
									  , _AcceptDroppedFiles AcceptDroppedFiles )
{
	if( AcceptDroppedFiles )
	{
		pc->AcceptDroppedFiles = (_AcceptDroppedFiles*)Reallocate( pc->AcceptDroppedFiles, ( pc->nAcceptDroppedFiles + 1 ) * sizeof( pc->AcceptDroppedFiles[0] ) );
		pc->AcceptDroppedFiles[pc->nAcceptDroppedFiles++] = AcceptDroppedFiles;
	}
}

//---------------------------------------------------------------------------

void SetCommonAcceptDroppedFiles( PSI_CONTROL pc
									  , _AcceptDroppedFiles AcceptDroppedFiles )
{
	if( AcceptDroppedFiles )
	{
		pc->AcceptDroppedFiles = (_AcceptDroppedFiles*)Preallocate( pc->AcceptDroppedFiles, ( pc->nAcceptDroppedFiles + 1 ) * sizeof( pc->AcceptDroppedFiles[0] ) );
		pc->AcceptDroppedFiles[0] = AcceptDroppedFiles;
		pc->nAcceptDroppedFiles++;
	}
}

//---------------------------------------------------------------------------

void AddCommonKey( PSI_CONTROL pc
									 ,int (CPROC*Key)(PSI_CONTROL,uint32_t) )
{
	if( Key )
	{
		pc->_KeyProc = (__KeyProc*)Reallocate( pc->_KeyProc, ( pc->n_KeyProc + 1 ) * sizeof( pc->_KeyProc[0] ) );
		pc->_KeyProc[pc->n_KeyProc++] = Key;
	}
}

//---------------------------------------------------------------------------

void SetCommonKey( PSI_CONTROL pc
									 ,int (CPROC*Key)(PSI_CONTROL,uint32_t) )
{
	if( Key )
	{
		pc->_KeyProc = (__KeyProc*)Preallocate( pc->_KeyProc, ( pc->n_KeyProc + 1 ) * sizeof( pc->_KeyProc[0] ) );
		pc->_KeyProc[0] = Key;
		pc->n_KeyProc++;
	}
}

//---------------------------------------------------------------------------

void SetCommonText(PSI_CONTROL pc, CTEXTSTR text )
{
	SetControlText( pc, text );
}

#undef ControlType
INDEX ControlType( PSI_CONTROL pc )
{
	if( pc )
		return pc->nType;
	return INVALID_INDEX;
}
//---------------------------------------------------------------------------

void SetControlTransparent( PSI_CONTROL pc, LOGICAL bTransparent )
{
	if( pc )
	{
		// if we are setting to transparent NO, then remove OriginalImage
		if( !(pc->flags.bTransparent = bTransparent ) )
		{
			//lprintf( "Turning off tansparency, so we don't need the background image now" );
			if( pc->OriginalSurface )
			{
				lprintf( "Early destruction of original surface image..." );
				UnmakeImageFile( pc->OriginalSurface );
				pc->OriginalSurface = NULL;
			}
		}
	}
}

//---------------------------------------------------------------------------

PSI_PROC( void, GetFramePosition )( PSI_CONTROL pf, int32_t *x, int32_t *y )
{
	if( pf )
	{
		PPHYSICAL_DEVICE pfd = pf->device;
		//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, pcf );
		if( pfd )
		{
			GetDisplayPosition( pfd->pActImg, x, y, NULL, NULL );
			return;
		}
		if( x )
			(*x) = pf->original_rect.x;
		if( y )
			(*y) = pf->original_rect.y;
	}
}

//---------------------------------------------------------------------------

PSI_PROC( void, GetFrameSize )( PSI_CONTROL pf, uint32_t *w, uint32_t *h )
{
	if( pf )
	{
		PPHYSICAL_DEVICE pfd = pf->device;
		//ValidatedControlData( PFRAME, CONTROL_FRAME, pf, pcf );
		if( pfd )
		{
			GetDisplayPosition( pfd->pActImg, NULL, NULL, w, h );
			(*w) -= FrameBorderX( pf, pf->BorderType );
			(*h) -= FrameBorderY( pf, pf->BorderType, GetText( pf->caption.text ) );
			return;
		}
		if( w )
			(*w) = pf->original_rect.width;
		if( h )
			(*h) = pf->original_rect.height;
	}
}

CTEXTSTR GetControlTypeName( PSI_CONTROL pc )
{
	TEXTCHAR mydef[32];
	if( !pc->pTypeName ) {
		tnprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY "/control/%d", pc->nType );
		return GetRegisteredValueExx( mydef, NULL, "type", FALSE );
	}
	else return pc->pTypeName;
}

void BeginUpdate( PSI_CONTROL pc )
{
	if( pc )
		pc->flags.bDirectUpdating = 1;
}

void EndUpdate( PSI_CONTROL pc )
{
	if( pc )
		pc->flags.bDirectUpdating = 0;
}

void SetCaptionChangedMethod( PSI_CONTROL frame, void (CPROC*_CaptionChanged)    (struct psi_common_control_frame *) )
{
	frame->CaptionChanged = _CaptionChanged;
}

void SetFrameDetachHandler( PSI_CONTROL pc, void ( CPROC*frameDetached )( struct psi_common_control_frame * pc ) ) {
	if( pc && pc->device ) {
		pc->device->EditState.frameDetached = frameDetached;
	}
}

void SetFrameEditDoneHandler( PSI_CONTROL pc, void ( CPROC*editDone )( struct psi_common_control_frame * pc ) ) {
	if( pc && pc->device ) {
		pc->device->EditState.frameEditDone = editDone;
	}
}

PSI_NAMESPACE_END

//---------------------------------------------------------------------------
