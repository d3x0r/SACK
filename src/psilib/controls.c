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

  CDATA TESTCOLOR;
typedef struct resource_names
{
	_32 resource_name_id;
	_32 resource_name_range;
	CTEXTSTR resource_name;
	CTEXTSTR type_name;
} RESOURCE_NAMES;
static RESOURCE_NAMES resource_names[] = {
#define BUILD_NAMES
#ifdef __cplusplus
#define FIRST_SYMNAME(name,control_type_name)  { name, 1, WIDE(#name), control_type_name }
#define SYMNAME(name,control_type_name)  , { name, 1, WIDE(#name), control_type_name }
#define SYMNAME_SKIP(prior,range,name,control_type_name)  , { prior, range, WIDE(#prior), control_type_name } \
	, { name, 1, WIDE(#name), control_type_name }
#else
#ifdef __WATCOMC__
#define FIRST_SYMNAME(name,control_type_name)  [name - FIRST_SYMBOL] = { name, 1, WIDE(#name), control_type_name }
#define SYMNAME(name,control_type_name)  , [name - FIRST_SYMBOL] = { name, 1, WIDE(#name), control_type_name }
#define SYMNAME_SKIP(prior,range,name,control_type_name)  , [prior - FIRST_SYMBOL] = { prior, range, WIDE(#prior), control_type_name } \
	, [name - FIRST_SYMBOL] = { name, 1, WIDE(#name), control_type_name }
#else
#define FIRST_SYMNAME(name,control_type_name)  { name, 1, WIDE(#name), control_type_name }
#define SYMNAME(name,control_type_name)  , { name, 1, WIDE(#name), control_type_name }
#define SYMNAME_SKIP(prior,range,name,control_type_name)  , { prior, range, WIDE(#prior), control_type_name } \
	, { name, 1, WIDE(#name), control_type_name }
#endif
#endif
#include "resource.h"
};

TEXTCHAR *GetResourceIDName( CTEXTSTR pTypeName, int ID )
{
	PCLASSROOT data = NULL;
	CTEXTSTR name;
	for( name = GetFirstRegisteredName( PSI_ROOT_REGISTRY WIDE("/resources/"), &data );
		  name;
		  name = GetNextRegisteredName( &data ) )
	{
		TEXTCHAR rootname[256];
		CTEXTSTR name2;
		PCLASSROOT data2 = NULL;
		snprintf( rootname, sizeof(rootname),PSI_ROOT_REGISTRY WIDE("/resources/%s/%s"), name, pTypeName );
		//lprintf( WIDE("newroot = %s"), rootname );
		for( name2 = GetFirstRegisteredName( rootname, &data2 );
			 name2;
			  name2 = GetNextRegisteredName( &data2 ) )
		{
			int value = (int)(long)(GetRegisteredValueExx( data2, name2, WIDE("value"), TRUE ));
			int range = (int)(long)(GetRegisteredValueExx( data2, name2, WIDE("range"), TRUE ));
			//lprintf( WIDE("Found Name %s"), name2 );
			if( (value <= ID) && ((value+range) > ID) )
			{
				size_t len;
				TEXTCHAR *result = NewArray( TEXTCHAR, len = strlen( pTypeName ) + strlen( name ) + strlen( name2 ) + 3 + 20 );
				if( value == ID )
					snprintf( result, len*sizeof(TEXTCHAR), WIDE("%s/%s/%s"), name, pTypeName, name2 );
				else
					snprintf( result, len*sizeof(TEXTCHAR), WIDE("%s/%s/%s+%d"), name, pTypeName, name2, ID-value );
				return result;
			}
		}
	}
	return NULL;
}


// also fix the name passed in?
int GetResourceID( PSI_CONTROL parent, CTEXTSTR name, _32 nIDDefault )
{
	if( !pathchr( name ) )
	{
		// assume search mode to find name... using name as terminal leef, but application and control class are omitted...
		PCLASSROOT data = NULL;
		CTEXTSTR name;
		//DebugBreak(); // changed 'name' to data and 'name2' to 'data2' ...
		for( name = GetFirstRegisteredName( PSI_ROOT_REGISTRY WIDE("/resources/"), &data );
			 name;
			  name = GetNextRegisteredName( &data ) )
		{
			//TEXTCHAR rootname[256];
			CTEXTSTR name2;
			PCLASSROOT data2 = NULL;
			//snprintf( rootname, sizeof(rootname),PSI_ROOT_REGISTRY "/resources/%s", name );
			//lprintf( WIDE("newroot = %s"), rootname );
			for( name2 = GetFirstRegisteredName( (CTEXTSTR)data, &data2 );
				 name2;
				  name2 = GetNextRegisteredName( &data2 ) )
			{
				TEXTCHAR rootname[256];
				int nResult;
				//lprintf( WIDE("Found Name %s"), name2 );
				snprintf( rootname, sizeof( rootname ), PSI_ROOT_REGISTRY WIDE("/resources/%s/%s"), name, name2 );
				if( GetRegisteredStaticIntValue( NULL, rootname, name, &nResult ) )
				{
					return nResult;
				}
				else
				{
					if( ( nIDDefault != -1 ) )
					{
						RegisterIntValue( rootname, WIDE("value"), nIDDefault );
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
			snprintf( buffer, sizeof( buffer ), PSI_ROOT_REGISTRY WIDE( "/resources/%s" ), name );
			range = (int)(long)GetRegisteredValueExx( (PCLASSROOT)PSI_ROOT_REGISTRY WIDE("/resources"), name, WIDE("range"), TRUE );
			result = (int)(long)GetRegisteredValueExx( (PCLASSROOT)PSI_ROOT_REGISTRY WIDE("/resources"), name, WIDE("value"), TRUE )/* + offset*/;
			if( !result && !range && ( nIDDefault != -1 ) )
			{
				RegisterIntValue( buffer, WIDE("value"), nIDDefault );
				RegisterIntValue( buffer, WIDE("range"), offset + 1 );
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
			lprintf( WIDE("Result of %s is %d"), name, result );
#endif
			return result;
		}
	}
	return -1; // TXT_STATIC id... invalid to search or locate on...
}

// yes, sometimes we end up having to register psi outside of preload...
// sometimes it works.
static void CPROC InitPSILibrary( void );

#undef DoRegisterControl
int DoRegisterControl( PCONTROL_REGISTRATION pcr, int nSize )
{
	if( pcr )
	{
#ifdef __cplusplus_cli
		// skip these well above legacy controls...
		// if we have 50 controls existing we'd be lucky
		// so creation of 1950 more controls shouldn't happen
		// within the timespan of xperdex.
		static _32 ControlID = USER_CONTROL + 2000;
#else
		static _32 ControlID = USER_CONTROL;
#endif
		TEXTCHAR namebuf[64], namebuf2[64];
		PCLASSROOT root;
		//lprintf( WIDE("Registering control: %s"), pcr->name );
		// okay do this so we get our names right?
		InitPSILibrary();
		//pcr->TypeID = ControlID;
		snprintf( namebuf2, sizeof( namebuf2 ), PSI_ROOT_REGISTRY WIDE("/control/%s")
				  , pcr->name );
		root = GetClassRoot( namebuf2 );
		pcr->TypeID = (int)(long)GetRegisteredValueExx( root, NULL, WIDE("Type"), TRUE );
		if( !pcr->TypeID && (StrCaseCmp( pcr->name, WIDE("FRAME") )!=0) )
		{
			ControlID = (_32)(PTRSZVAL)GetRegisteredValueExx( WIDE("PSI/Controls"), NULL, WIDE("User Type ID"), TRUE);
			if( !ControlID )
			{
#ifdef __cplusplus_cli
				ControlID = USER_CONTROL + 2000;
#else
				ControlID = USER_CONTROL;
#endif
			}

			pcr->TypeID = ControlID;
			snprintf( namebuf, sizeof( namebuf ), PSI_ROOT_REGISTRY WIDE("/control/%") _32f
					  , ControlID );
			root = RegisterClassAlias( namebuf2, namebuf );
			RegisterValueExx( root, NULL, WIDE("Type"), FALSE, pcr->name );
			RegisterValueExx( root, NULL, WIDE("Type"), TRUE, (CTEXTSTR)(PTRSZVAL)ControlID );
			snprintf( namebuf, sizeof( namebuf ), PSI_ROOT_REGISTRY_OTHER WIDE("/control/%") _32f
					  , ControlID );
			root = RegisterClassAlias( namebuf2, namebuf );
			snprintf( namebuf, sizeof( namebuf ), PSI_ROOT_REGISTRY_OTHER WIDE("/control/%s")
				, pcr->name );
			root = RegisterClassAlias( namebuf2, namebuf );
			ControlID++; // new control type registered...
			RegisterIntValueEx( (PCLASSROOT)WIDE("PSI/Controls"), NULL, WIDE("User Type ID"), (PTRSZVAL)ControlID );
		}
		else
		{
			//char longbuf[128];
			//snprintf( longbuf, sizeof( longbuf ), PSI_ROOT_REGISTRY "/control/%s/rtti", pcr->name );
			//if( CheckClassRoot( longbuf ) )
			{
				//lprintf( "Aborting second registration fo same type." );
				//return pcr->TypeID;
			}
		}
#define EXTRA2 stuff.stuff.
#define EXTRA stuff.
		RegisterIntValueEx( root, NULL, WIDE("extra"), pcr->EXTRA extra );
		RegisterIntValueEx( root, NULL, WIDE("width"), pcr->EXTRA2 width );
		RegisterIntValueEx( root, NULL, WIDE("height"), pcr->EXTRA2 height );
		RegisterIntValueEx( root, NULL, WIDE("border"), pcr->EXTRA default_border );
		// root will now be /psi/controls/(name)/rtti/...=...
		root = GetClassRootEx( root, WIDE("rtti") );
		if( pcr->init )
			SimpleRegisterMethod( root, pcr->init
									  , WIDE("int"), WIDE("init"), WIDE("(PSI_CONTROL,va_list)") );
		if( pcr->load )
			SimpleRegisterMethod( root, pcr->load
									  , WIDE("int"), WIDE("load"), WIDE("(PSI_CONTROL,PTEXT)") );
		if( pcr->draw )
			SimpleRegisterMethod( root, pcr->draw
									  , WIDE("void"), WIDE("draw"), WIDE("(PSI_CONTROL)") );
		if( pcr->mouse )
			SimpleRegisterMethod( root, pcr->mouse
									  , WIDE("void"), WIDE("mouse"), WIDE("(PSI_CONTROL,S_32,S_32,_32)") );
		if( pcr->key )
			SimpleRegisterMethod( root, pcr->key
									  , WIDE("void"), WIDE("key"), WIDE("(PSI_CONTROL,_32)") );
		if( pcr->destroy )
			SimpleRegisterMethod( root, pcr->destroy
									  , WIDE("void"), WIDE("destroy"), WIDE("(PSI_CONTROL)") );
		if( pcr->save )
			SimpleRegisterMethod( root, pcr->save
									  , WIDE("void"), WIDE("save"), WIDE("(PSI_CONTROL,PVARTEXT)") );
		if( pcr->prop_page )
			SimpleRegisterMethod( root, pcr->prop_page
									  , WIDE("PSI_CONTROL"), WIDE("get_prop_page"), WIDE("(PSI_CONTROL)") );
		if( pcr->apply_prop )
			SimpleRegisterMethod( root, pcr->apply_prop
									  , WIDE("void"), WIDE("apply"), WIDE("(PSI_CONTROL,PSI_CONTROL)") );
		if( pcr->CaptionChanged )
			SimpleRegisterMethod( root, pcr->CaptionChanged
									  , WIDE("void"), WIDE("caption_changed"), WIDE("(PSI_CONTROL)") );
		if( pcr->FocusChanged )
			SimpleRegisterMethod( root, pcr->FocusChanged
									  , WIDE("void"), WIDE("focus_changed"), WIDE("(PSI_CONTROL,LOGICAL)") );
		if( pcr->AddedControl )
			SimpleRegisterMethod( root, pcr->AddedControl
									  , WIDE("void"), WIDE("add_control"), WIDE("(PSI_CONTROL,PSI_CONTROL)") );
		if( nSize > ( offsetof( CONTROL_REGISTRATION, PositionChanging ) + sizeof( pcr->PositionChanging ) ) )
		{
			if( pcr->PositionChanging )
				SimpleRegisterMethod( root, pcr->PositionChanging
										  , WIDE("void"), WIDE("position_changing"), WIDE("(PSI_CONTROL,LOGICAL)") );
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
		g.MyImageInterface = (PIMAGE_INTERFACE)GetInterface( WIDE("image") );
		if( !g.MyImageInterface )
			g.MyImageInterface = (PIMAGE_INTERFACE)GetInterface( WIDE("real_image") );
		if( !g.MyImageInterface )
		{
#ifndef XBAG
			//if( is_deadstart_complete() )
#endif
			{
#ifndef WIN32
				fprintf( stderr, "Failed to get 'image' interface.  PSI interfaces failing execution." );
#endif
				lprintf( WIDE("Failed to get 'image' interface.  PSI interfaces failing execution.") );
				lprintf( WIDE("and why yes, if we had a display, I suppose we could allow someone to fix this problem in-line...") );
				lprintf( WIDE("-------- DIE - DIE - DIE - DIE - DIE - DIE - DIE - DIE - DIE - DIE - DIE ---------") );
				//DebugBreak();
				//exit(0);
			}
			lprintf( WIDE("Fail image load once...") );
		}
		else
		{
			if( !DefaultColors[0] )
			{
				DefaultColors[0] = Color( 192, 192, 192 ); // highlight
				DefaultColors[1] = AColor( 53, 96, 89, 255 ); // normal
				DefaultColors[2] = Color( 35, 63, 57 ); // shade
				DefaultColors[3] = Color( 0, 0, 1 ); // shadow
				DefaultColors[4] = Color( 0, 240, 240 ); // text
				DefaultColors[5] = Color( 88,124,200 ); // caption background
				DefaultColors[6] = Color( 240, 240, 240 ); // cannot be black(0). caption text
				DefaultColors[7] = Color( 89, 120, 120 ); // inactive caption background
				DefaultColors[8] = Color( 0, 0, 1 );     // inactive caption text (not black)
				DefaultColors[9] = Color( 0, 0, 128 );  // text select background
				DefaultColors[10] = Color( 220, 220, 220 ); // text select foreground
				DefaultColors[11] = AColor( 192, 192, 192, 255 ); // edit background
				DefaultColors[12] = Color( 0, 0, 1 );  // edit text
				DefaultColors[13] = Color( 120, 120, 180 ); // scroll bar...
			}
		}
	}
	if( !g.MyDisplayInterface )
	{
		g.MyDisplayInterface = (PRENDER_INTERFACE)GetInterface( WIDE("render") );
		if( !g.MyImageInterface )
			g.MyDisplayInterface = (PRENDER_INTERFACE)GetInterface( WIDE("video") );
		if( !g.MyDisplayInterface )
		{
			{
#ifndef WIN32
				fprintf( stderr, "Failed to get 'render' interface.  PSI interfaces failing execution." );
#endif
				lprintf( WIDE("Failed to get 'render' interface.  PSI interfaces failing execution.") );
				lprintf( WIDE("and why yes, if we had a display, I suppose we could allow someone to fix this problem in-line...") );
				//exit(0);
			}
			lprintf( WIDE("Fail render load once...") );
		}
		else
			if( !(g.flags.always_draw = RequiresDrawAll()) )
				g.flags.allow_threaded_draw = AllowsAnyThreadToUpdate();
	}
#ifdef __ANDROID__
	{
		_32 w, h;
		GetDisplaySize( &w, &h );
		g.default_font = RenderFontFileScaledEx( WIDE("fonts/MyriadPro.ttf"), w / 58, h / 32, NULL, NULL, 2/*FONT_FLAG_8BIT*/, NULL, NULL );
	}
#endif
}
#endif

//--------------------------------------------------------------------------

#ifdef __WATCOM_CPLUSPLUS__
#pragma initialize 47
#endif
PRIORITY_PRELOAD( InitPSILibrary, PSI_PRELOAD_PRIORITY )
{
	//static int bInited;
	if( !GetRegisteredIntValue( PSI_ROOT_REGISTRY WIDE("/init"), WIDE("done") ) )
	//if( !bInited )

	{
		TEXTCHAR namebuf[64], namebuf2[64];

#define REG(name) {   \
			snprintf( namebuf, sizeof( namebuf ), PSI_ROOT_REGISTRY WIDE("/control/%d"), name ); \
			snprintf( namebuf2, sizeof( namebuf2 ), PSI_ROOT_REGISTRY WIDE("/control/%s"), name##_NAME );             \
			RegisterClassAlias( namebuf2, namebuf );  \
			RegisterValue( namebuf2, WIDE("Type"), name##_NAME ); \
			RegisterIntValue( namebuf2, WIDE("Type"), name ); \
		}
#define REG2(name,number) {   \
			snprintf( namebuf, sizeof( namebuf ), PSI_ROOT_REGISTRY WIDE("/control/%d"), number ); \
			snprintf( namebuf2, sizeof( namebuf2 ), PSI_ROOT_REGISTRY WIDE("/control/%s"), name );             \
			RegisterClassAlias( namebuf2, namebuf );  \
			RegisterValue( namebuf2, WIDE("Type"), name ); \
			RegisterIntValue( namebuf2, WIDE("Type"), number ); \
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

		REG2(WIDE("Color Matrix"), BUILTIN_CONTROL_COUNT + 0 );
		REG2(WIDE("Font Sample"), BUILTIN_CONTROL_COUNT + 1 );
		REG2(WIDE("Font Size Control"), BUILTIN_CONTROL_COUNT + 2 );
		REG2(WIDE("Popup Menu"), BUILTIN_CONTROL_COUNT + 3 );
		REG2(WIDE("Basic Clock Widget"), BUILTIN_CONTROL_COUNT + 4 );
		REG2(WIDE("Scroll Knob"), BUILTIN_CONTROL_COUNT + 5 );
		REG2(WIDE("PSI Console"), BUILTIN_CONTROL_COUNT + 6 );
		REG2(WIDE("Shade Well"), BUILTIN_CONTROL_COUNT + 7 );
		REG2(WIDE("Color Well"), BUILTIN_CONTROL_COUNT + 8 );
		{
			int nResources = sizeof( resource_names ) / sizeof( resource_names[0] );
			int n;
			for( n = 0; n < nResources; n++ )
			{
				if( resource_names[n].resource_name_id )
				{
					TEXTCHAR root[256];
					TEXTCHAR old_root[256];
#define TASK_PREFIX WIDE( "core" )
					snprintf( root, sizeof( root )
							  , PSI_ROOT_REGISTRY WIDE("/resources/%s/") TASK_PREFIX WIDE("/%s")
							  , resource_names[n].type_name
							  , resource_names[n].resource_name );
					RegisterIntValue( root
										 , WIDE("value")
										 , resource_names[n].resource_name_id );
					RegisterIntValue( root
										 , WIDE("range")
										 , resource_names[n].resource_name_range );
					snprintf( root, sizeof( root ), PSI_ROOT_REGISTRY WIDE("/resources/%s/") TASK_PREFIX WIDE(""), resource_names[n].type_name );
					snprintf( old_root, sizeof( old_root ), PSI_ROOT_REGISTRY WIDE("/resources/") TASK_PREFIX WIDE("/%s"), resource_names[n].type_name );
					RegisterIntValue( root
										 , resource_names[n].resource_name
										 , resource_names[n].resource_name_id );
					RegisterClassAlias( root, old_root );
				}
			}
		}

		RegisterIntValue( PSI_ROOT_REGISTRY WIDE("/init"), WIDE("done"), 1 );
	}
}



void TryLoadingFrameImage( void )
{
	if( !g.BorderImage )
	{
		TEXTCHAR buffer[256];
#ifndef __NO_OPTIONS__
		SACK_GetProfileStringEx( GetProgramName(), WIDE( "SACK/PSI/Frame border image" ), WIDE("frame_border.png"), buffer, sizeof( buffer ), TRUE );
#else
		StrCpy( buffer, WIDE("frame_border.png") );
#endif
		g.BorderImage = LoadImageFileFromGroup( GetFileGroup( WIDE( "Images" ), WIDE( "./images" ) ), buffer );
		if( g.BorderImage )
		{
			int MiddleSegmentWidth, MiddleSegmentHeight;
			if( g.BorderImage->width & 1 )
				g.BorderWidth = g.BorderImage->width / 2;
			else
				g.BorderWidth = (g.BorderImage->width-1) / 2;
			if( g.BorderImage->height )
				g.BorderHeight = g.BorderImage->height / 2;
			else
				g.BorderHeight = (g.BorderImage->height-1) / 2;
			MiddleSegmentWidth = g.BorderImage->width - (g.BorderWidth*2);
			MiddleSegmentHeight = g.BorderImage->height - (g.BorderHeight*2);
			g.BorderSegment[SEGMENT_TOP_LEFT] = MakeSubImage( g.BorderImage, 0, 0, g.BorderWidth, g.BorderHeight );
			g.BorderSegment[SEGMENT_TOP] = MakeSubImage( g.BorderImage, g.BorderWidth, 0
																	 , MiddleSegmentWidth, g.BorderHeight );
			g.BorderSegment[SEGMENT_TOP_RIGHT] = MakeSubImage( g.BorderImage, g.BorderWidth + MiddleSegmentWidth, 0, g.BorderWidth, g.BorderHeight );
			g.BorderSegment[SEGMENT_LEFT] = MakeSubImage( g.BorderImage
																	  , 0, g.BorderHeight
																	  , g.BorderWidth, MiddleSegmentHeight );
			g.BorderSegment[SEGMENT_CENTER] = MakeSubImage( g.BorderImage
																		 , g.BorderWidth, g.BorderHeight
																		 , MiddleSegmentWidth, MiddleSegmentHeight );
			SetBaseColor( NORMAL, getpixel( g.BorderSegment[SEGMENT_CENTER], 0, 0 ) );
			g.BorderSegment[SEGMENT_RIGHT] = MakeSubImage( g.BorderImage
																		, g.BorderWidth + MiddleSegmentWidth, g.BorderHeight
																		, g.BorderWidth, MiddleSegmentHeight );
			g.BorderSegment[SEGMENT_BOTTOM_LEFT] = MakeSubImage( g.BorderImage
																				, 0, g.BorderHeight + MiddleSegmentHeight
																				, g.BorderWidth, g.BorderHeight );
			g.BorderSegment[SEGMENT_BOTTOM] = MakeSubImage( g.BorderImage
																		 , g.BorderWidth, g.BorderHeight + MiddleSegmentHeight
																	 , MiddleSegmentWidth, g.BorderHeight );
			g.BorderSegment[SEGMENT_BOTTOM_RIGHT] = MakeSubImage( g.BorderImage
																				 , g.BorderWidth + MiddleSegmentWidth, g.BorderHeight + MiddleSegmentHeight
																				 , g.BorderWidth, g.BorderHeight );
		}
	}
}

// this can be run very late...
PRELOAD( DefaultControlStartup )
{
#ifndef __NO_OPTIONS__
	g.flags.bLogDebugUpdate = SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/PSI/Log Control Updates" ), 0, TRUE );
	g.flags.bLogDetailedMouse = SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/PSI/Log Mouse Events" ), 0, TRUE );
	g.flags.bLogKeyEvents = SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/PSI/Log Key Events" ), 0, TRUE );
	g.flags.bLogSuperDetailedMouse = SACK_GetProfileIntEx( GetProgramName(), WIDE( "SACK/PSI/Log Mouse Events extra detail" ), 0, TRUE );
#endif
}

PSI_PROC( PIMAGE_INTERFACE, SetControlImageInterface )( PIMAGE_INTERFACE DisplayInterface )
{
#ifndef PSI_SERVICE
#  ifdef USE_INTERFACES
	return g.MyImageInterface = DisplayInterface;
#endif
#endif
	return DisplayInterface;
}


PSI_PROC( PRENDER_INTERFACE, SetControlInterface )( PRENDER_INTERFACE DisplayInterface )
{
#ifndef PSI_SERVICE
#  ifdef USE_INTERFACES
	return g.MyDisplayInterface = DisplayInterface;
#endif
#endif
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

#define Swap(i)    ( (tmp = i),( sys_r = ((tmp) & 0xFF)), (sys_g = ((tmp>>8) & 0xFF)),(sys_b = ((tmp >>16) & 0xFF)),(sys_a = 0xFF),AColor(sys_r,sys_g,sys_b,sys_a) )

	 defaultcolor[HIGHLIGHT        ] =  Swap(GetSysColor( COLOR_3DHIGHLIGHT));
	if( !g.BorderImage )
		 defaultcolor[NORMAL           ] =  Swap(GetSysColor(COLOR_3DFACE ));
	defaultcolor[SHADE            ] =  Swap(GetSysColor(COLOR_3DSHADOW ));
	defaultcolor[SHADOW           ] =  Swap(GetSysColor(COLOR_3DDKSHADOW ));
	defaultcolor[TEXTCOLOR        ] =  Swap(GetSysColor(COLOR_BTNTEXT ));
	defaultcolor[CAPTION          ] =  Swap(GetSysColor(COLOR_ACTIVECAPTION ));
	defaultcolor[CAPTIONTEXTCOLOR] =  Swap(GetSysColor( COLOR_CAPTIONTEXT));
	defaultcolor[INACTIVECAPTION ] =  Swap(GetSysColor(COLOR_INACTIVECAPTION ));    
	defaultcolor[INACTIVECAPTIONTEXTCOLOR]=Swap(GetSysColor(COLOR_INACTIVECAPTIONTEXT ));
	defaultcolor[SELECT_BACK      ] =  Swap(GetSysColor(COLOR_HIGHLIGHT ));
	defaultcolor[SELECT_TEXT      ] =  Swap(GetSysColor(COLOR_HIGHLIGHTTEXT ));
	defaultcolor[EDIT_BACKGROUND ] =  Swap(GetSysColor(COLOR_WINDOW ));
	defaultcolor[EDIT_TEXT       ] =  Swap(GetSysColor(COLOR_WINDOWTEXT ));
	defaultcolor[SCROLLBAR_BACK  ] =  Swap(GetSysColor(COLOR_SCROLLBAR ));
#endif
    // No base to set to - KDE/Gnome/E/?
}

//---------------------------------------------------------------------------

PSI_PROC( void, SetBaseColor )( INDEX idx, CDATA c )
{
	//Log3( WIDE("Color %d was %08X and is now %08X"), idx, defaultcolor[idx], c );
	defaultcolor[idx] = c;
}

PSI_PROC( CDATA, GetBaseColor )( INDEX idx )
{
	return defaultcolor[idx] ;
}

//---------------------------------------------------------------------------

PSI_PROC( void, SetControlColor )( PSI_CONTROL pc, INDEX idx, CDATA c )
{
	//Log3( WIDE("Color %d was %08X and is now %08X"), idx, basecolor(pc)[idx], c );
	if( pc )
	{
		if( basecolor(pc) == g.defaultcolors )
		{
			pc->basecolors = NewArray( CDATA, sizeof( DefaultColors ) / sizeof( CDATA ) );
			MemCpy( pc->basecolors, g.defaultcolors, sizeof( DefaultColors ) );
		}
		basecolor(pc)[idx] = c;
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
		lprintf( WIDE("FixFrameFocus....") );
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
					if( (!pcCurrent->flags.bNoFocus) &&
						((!pcCurrent->flags.bDisable) ||
						 (pcCurrent == pcStart )))
					{
						if( pcCurrent != pcStart )
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
				lprintf( WIDE( "Restoring orignal background... " ) );
#endif
			pc->flags.bParentCleaned = 1;
			BlotImageSizedTo( pc->Surface, pc->OriginalSurface,  r->x, r->y, r->x, r->y, r->width, r->height );
		}
		else
		{
			lprintf( WIDE("parent would have to draw before I can restore my control's background") );
		}
	}

}

//---------------------------------------------------------------------------

void InvokeControlHidden( PSI_CONTROL pc )
{
	void (CPROC *OnHidden)(PSI_CONTROL);
	PCLASSROOT data = NULL;
	PCLASSROOT event_root = GetClassRootEx( pc->class_root, WIDE( "hide_control" ) );
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
	PCLASSROOT event_root = GetClassRootEx( pc->class_root, WIDE( "reveal_control" ) );
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
		lprintf( WIDE( "Update Some Controls - (all controls within rect %d) " ), pc->flags.bInitial );
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
		lprintf( WIDE("Control is hidden, skipping it.") );
		return;
		//continue;
	}
	//lprintf( WIDE("UpdateSomeControls - input rect is %d,%d  %d,%d"), pRect->x, pRect->y, pRect->width, pRect->height );
	//lprintf( WIDE("UpdateSomeControls - changed rect is %d,%d  %d,%d"), pRect->x, pRect->y, pRect->width, pRect->height );

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
				lprintf( WIDE("Some controls using normal updatecommon to draw...") );
#endif
			// enabled minimal update region...
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
			{
				lprintf( WIDE("Blatting color to surface so that we have something update?!") );
				lprintf( WIDE("Update portion %d,%d to %d,%d"), surf_rect.x, surf_rect.y, surf_rect.width, surf_rect.height );
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
					lprintf( WIDE("a control %p (self, or some parent %p) has %s or %s")
							  , pc, parent
							  , parent->flags.bNoUpdate?WIDE("noupdate"):WIDE("...")
							  , parent->flags.bHidden?WIDE("hidden"):WIDE("...")
							  );
					break;
				}
			}
			if( parent )
			{
				lprintf( WIDE("ABORTING SMUDGE") );
				continue;
			}
		}
		if( pc->flags.bHidden || pc->flags.bNoUpdate )
		{
			lprintf( WIDE("Control is hidden, skipping it.") );
			continue;
		}
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			lprintf( WIDE("updating some controls... rectangles and stuff.") );
#endif
	 //Log( WIDE("Update some controls....") );
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
					lprintf( WIDE("Some controls using normal updatecommon to draw...") );
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
				//Log( WIDE("Hit the rectange, but didn't hit the content... so update border only.") );
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
						lprintf( WIDE("Drew border, drawing caption uhmm update some work controls") );
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
	//lprintf( WIDE("SmudgeSomeControls - input rect is %d,%d  %d,%d"), pRect->x, pRect->y, pRect->width, pRect->height );
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
	//lprintf( WIDE("SmudgeSomeControls - changed rect is %d,%d  %d,%d"), pRect->x, pRect->y, pRect->width, pRect->height );
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
			lprintf( WIDE("Blatting color to surface so that we have something update?!") );
			lprintf( WIDE("Update portion %d,%d to %d,%d"), pRect->x, pRect->y, pRect->width, pRect->height );
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

static int OnDrawCommon( WIDE("Frame") )( PSI_CONTROL pc )
{
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		lprintf( WIDE( "-=-=-=-=- Output Frame background..." ) );
#endif
	BlatColorAlpha( pc->Surface, 0, 0, pc->surface_rect.width, pc->surface_rect.height, basecolor(pc)[NORMAL] );
	DrawFrameCaption( pc );
	return 1;
}

//--------------------------------------------------------------------------

static void OnDrawCommonDecorations( WIDE("Frame") )( PSI_CONTROL pc )
{
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		lprintf( WIDE( "-=-=-=-=- Output Frame decorations..." ) );
#endif
	if( pc->device )
		DrawHotSpots( pc, &pc->device->EditState );
}

//--------------------------------------------------------------------------
// forward declaration cause we're lazy and don't want to re-wind the below routines...
typedef struct penging_rectangle_tag
{
	struct {
		BIT_FIELD bHasContent : 1;
		BIT_FIELD bTmpRect : 1;
		BIT_FIELD bInitialized : 1;
	} flags;
   // this shouldn't be a thread-shared structure.
	//CRITICALSECTION cs;
	S_32 x, y;
   _32 width, height;
} PENDING_RECT, *PPENDING_RECT;
static void DoUpdateCommonEx( PPENDING_RECT upd, PSI_CONTROL pc, int bDraw, int level DBG_PASS );

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
		lprintf( WIDE( "Why did ypu pass a NULL control to this?! ( the event to close happeend before updat" ) );
		return;
	}
#if DEBUG_UPDAATE_DRAW > 2
	if( g.flags.bLogDebugUpdate )
		_lprintf(DBG_RELAY)( WIDE( "Do Update frame.. x, y on frame of %d,%d,%d,%d " ), x, y, w, h );
#endif
	level++;
	if( pc && !pf )
	{
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			lprintf( WIDE("Stepping to parent, adding my surface rect and my rect to the coordinate updated... %d+%d+%d %d+%d+%d")
					 , x, pc->parent->rect.x, pc->parent->surface_rect.x
					 , y, pc->parent->rect.y, pc->parent->surface_rect.y
					 );
#endif
		if( pc->parent && !pc->device ) // otherwise we're probably still creating the thing, and it's in bInitial?
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE( "stepping to parent, assuming I'm copying my surface, so update appropriately" ) );
#endif
			TESTCOLOR=SetAlpha( BASE_COLOR_BLUE, 128 );
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
				lprintf( WIDE("Failing to update to screen cause %s and/or %s"), pc->flags.bInitial?WIDE( "it's initial" ):WIDE( "" )
						 , pc->flags.bNoUpdate ?WIDE( "it's no update..." ):WIDE( "..." ));
#endif
			level--;
			return;
		}

		{
			int bias_x = 0;
			int bias_y = 0;
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				_xlprintf( 1 DBG_RELAY )( WIDE("updating display portion %d,%d (%d,%d)")
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
		_lprintf(DBG_RELAY)( WIDE( "Update Frame (Another flavor ) %p %p  %d,%d %d,%d" ), pc, pf, x, y, w, h );
#endif
   //_xlprintf( 1 DBG_RELAY )( WIDE("Update Frame ------------") );
	if( pc && !pf )
	{
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
		{
			//lprintf( WIDE("Stepping to parent, adding my surface rect and my rect to the coordinate updated...") );
			lprintf( WIDE( "stepping to parent, assuming I'm copying my surface, so update appropriately" ) );
			lprintf( WIDE("Stepping to parent, adding my surface rect and my rect to the coordinate updated... %d+%d+%d %d+%d+%d")
					 , x, pc->parent->rect.x, pc->parent->surface_rect.x
					 , y, pc->parent->rect.y, pc->parent->surface_rect.y
					 );
		}
#endif
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
				lprintf( WIDE("Faileing to update to screen cause we're initial or it's no update...") );
#endif
			return;
		}

#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			_xlprintf( 1 DBG_RELAY )( WIDE("updating display portion %d,%d (%d,%d)")
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
		PENDING_RECT upd;
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
		{
			_lprintf( DBG_RELAY )( WIDE("Intelligent update? %p"), pc );
			lprintf( WIDE("pc = %p par = %p"), pc, pc->parent );
		}
#endif
		upd.flags.bHasContent = 0;
		upd.flags.bTmpRect = 0;
		//lprintf( WIDE("doing update common...") );
		DoUpdateCommonEx( &upd, pc, FALSE, 0 DBG_RELAY);
		if( upd.flags.bHasContent )
		{
			PSI_CONTROL frame;
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE("Updated all commons? %d,%d  %d,%d"), upd.x,upd.y, upd.width, upd.height );
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

void AddCommonUpdateRegionEx( PPENDING_RECT update_rect, int bSurface, PSI_CONTROL pc DBG_PASS )
#define AddCommonUpdateRegion(upd,surface,pc) AddCommonUpdateRegionEx( upd,surface,pc DBG_SRC )
{
	PSI_CONTROL parent;
	S_32 x, y;
	_32 wd, ht;
	if( !pc )
		return;
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		_lprintf(DBG_RELAY)( WIDE( "Adding region.... (mayfbe should wait)" ) );
#endif
	if( !pc->parent && pc->flags.bRestoring )
	{
		if( g.flags.bLogDebugUpdate )
			lprintf( WIDE( "no parent and not restoring." ) );
		wd = pc->rect.width;
		ht = pc->rect.height;
		x = 0;//pc->rect.x;
		y = 0;//pc->rect.y;
		pc->flags.bUpdateRegionSet = 0;
	}
	else if( pc->flags.bUpdateRegionSet )
	{
		if( g.flags.bLogDebugUpdate )
			lprintf( WIDE( "someone already set the region... " ) );
		wd = pc->update_rect.width;
		ht = pc->update_rect.height;
		x = pc->update_rect.x + pc->surface_rect.x;
		y = pc->update_rect.y + pc->surface_rect.y;
		pc->flags.bUpdateRegionSet = 0;
	}
	else
	{
		if( g.flags.bLogDebugUpdate )
			lprintf( WIDE( "parent and this is restoring " ) );
		if( bSurface )
		{
			//lprintf( WIDE("Computing control's surface rectangle.") );
			wd = pc->surface_rect.width;
			ht = pc->surface_rect.height;
		}
		else
		{
			//lprintf( WIDE("Computing control's window rectangle.") );
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
			lprintf( WIDE( "control has parent..." ) );

		for( parent = pc->parent; parent /*&& parent->parent*/; parent = parent->parent )
		{
			x += ((parent->parent&&!parent->device)?parent->rect.x:0) + parent->surface_rect.x;
			y += ((parent->parent&&!parent->device)?parent->rect.y:0) + parent->surface_rect.y;
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE( "control's parent makes x=%d and y=%d" ), x, y );
			if( parent->device )
				break;
		}
		//x += parent->surface_rect.x;
		//y += parent->surface_rect.y;
	}
	//else
   //   parent = pc;
   //lprintf( WIDE("Adding update region (%d,%d)-(%d,%d)"), x, y, wd, ht );
	if( wd && ht )
	{
		if( update_rect->flags.bHasContent )
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE("Adding (%d,%d)-(%d,%d) to (%d,%d)-(%d,%d)")
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
			if( ( x + (S_32)wd ) > ( update_rect->x + (S_32)update_rect->width ) )
				update_rect->width = ( (S_32)wd + x ) - update_rect->x;

			if( y < update_rect->y )
			{
				update_rect->height += update_rect->y - y;
				update_rect->y = y;
			}
			if( y + (S_32)ht > update_rect->y + (S_32)update_rect->height )
				update_rect->height = ( y + (S_32)ht ) - update_rect->y;
			//lprintf( WIDE(fs"result (%d,%d)-(%d,%d)")
		 //       , update_rect->x, update_rect->y
		 //       , update_rect->width, update_rect->height
			//		 );
		}
		else
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				_lprintf(DBG_RELAY)( WIDE("Setting (%d,%d)-(%d,%d)")
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

void SetUpdateRegionEx( PSI_CONTROL pc, S_32 rx, S_32 ry, _32 rw, _32 rh DBG_PASS )
{
	PSI_CONTROL parent;
	S_32 x, y;
	_32 wd, ht;
	if( !pc )
		return;


	//lprintf( WIDE("Computing control's surface rectangle.") );
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
   //lprintf( WIDE("Adding update region (%d,%d)-(%d,%d)"), x, y, wd, ht );
	if( wd && ht )
	{
		pc->flags.bUpdateRegionSet = 1;
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				_lprintf(DBG_RELAY)( WIDE("Setting (%d,%d)-(%d,%d)")
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

Image CopyOriginalSurfaceEx( PCONTROL pc, Image use_image DBG_PASS )
{
	Image copy;
	if( !pc )
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
		//lprintf( WIDE("################ COPY SURFACE ###################### ") );
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
static void DoUpdateCommonEx( PPENDING_RECT upd, PSI_CONTROL pc, int bDraw, int level DBG_PASS )
{
	int cleaned = 0;
	if( pc )
	{
		if( pc->parent )
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
					lprintf( WIDE( "out of bounds? %d,%d %d,%d  %d,%d %d,%d" ), pc->original_rect.x, pc->original_rect.y, pc->original_rect.width, pc->original_rect.height, pc->parent->surface_rect.width, pc->parent->surface_rect.height, /*pc->parent->surface_rect.x*/-pc->original_rect.x, /*pc->parent->surface_rect.y*/-pc->original_rect.y );
				}
				return;
			}
		}
#if DEBUG_UPDAATE_DRAW > 2
		if( g.flags.bLogDebugUpdate )
			_lprintf(DBG_RELAY)( WIDE( ">>Control updating %p(parent:%p,child:%p)" ), pc, pc->parent, pc->child );
#endif
		if( pc->flags.bNoUpdate /*|| !GetImageSurface(pc->Surface)*/ )
		{
#if DEBUG_UPDAATE_DRAW > 2
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE( "Control update is disabled" ) );
#endif
			return;
		}
#if DEBUG_UPDAATE_DRAW > 3
		if( g.flags.bLogDebugUpdate )
		{
			// might filter this to just if dirty, we get called a lot without dirty controls
			lprintf( WIDE("Control %p(%s) is %s and parent is %s"), pc, pc->pTypeName, pc->flags.bDirty?WIDE( "SMUDGED" ):WIDE( "clean" ), pc->flags.bParentCleaned?WIDE( "cleaned to me" ):WIDE( "dirty to me" ) );
			// again might filter to just forced...
			_xlprintf(LOG_NOISE DBG_RELAY )( WIDE(">>do draw... %p %p %s %s"), pc, pc->child
			                               , bDraw?WIDE( "FORCE" ):WIDE( "..." )
			                               , pc->flags.bCleaning?WIDE( "CLEANING" ):WIDE( "cleanable" ));
		}
#endif
		//#endif
		if( !pc->flags.bCleaning )
		{
#if DEBUG_UPDAATE_DRAW > 2
			if( g.flags.bLogDebugUpdate )
				_xlprintf(1 DBG_RELAY)( WIDE(">>Begin control %p update forced?%s"), pc, bDraw?WIDE( "Yes" ):WIDE( "No" ) );
#endif
			pc->flags.bCleaning = 1;
		retry_update:
			if( pc->parent && !pc->device )
			{
				// if my parent is initial, become initial also...
				//lprintf( WIDE( " Again, relaying my initial status..." ) );
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
					lprintf( WIDE("Control draw %p %s parent_clean?%d transparent?%d")
							 , pc, pc->pTypeName
							 , pc->flags.bParentCleaned
							 , pc->flags.bTransparent );
#endif
				if( !g.flags.always_draw && !IsImageTargetFinal( pc->Window ) )
				{
					if( ( ((pc->parent&&!pc->device) && pc->parent->flags.bDirty ) || pc->flags.bParentCleaned ) && pc->flags.bTransparent )//&& pc->flags.bFirstCleaning )
					{
						Image OldSurface;
						OldSurface = CopyOriginalSurface( pc, pc->OriginalSurface );
						if( OldSurface )
						{
#ifdef DEBUG_UPDAATE_DRAW
							if( g.flags.bLogDebugUpdate )
								_lprintf(DBG_RELAY)( WIDE("--------------- Successfully copied new background original image") );
#endif
							pc->OriginalSurface = OldSurface;
						}
						else
							if( pc->OriginalSurface )
							{
#ifdef DEBUG_UPDAATE_DRAW
								if( g.flags.bLogDebugUpdate )
									_xlprintf(LOG_NOISE DBG_RELAY)( WIDE("--------------- Restoring prior image (didn't need a new image)") );
								if( g.flags.bLogDebugUpdate )
									lprintf( WIDE( "Restoring orignal background... " ) );
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
								_xlprintf(LOG_NOISE DBG_RELAY)( WIDE("--------------- Restoring prior image") );
							if( g.flags.bLogDebugUpdate )
								lprintf( WIDE( "Restore original background..." ) );
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
				if( g.flags.bLogDebugUpdate )
					lprintf( WIDE("Invoking a draw self for %p at %s(%d) level %d"), pc DBG_RELAY , level );
#endif
				if( pc->flags.bDestroy )
					return;
				if( pc->flags.bTransparent && !pc->flags.bParentCleaned && (pc->parent&&!pc->device) )
				{
#ifdef DEBUG_UPDAATE_DRAW
					if( g.flags.bLogDebugUpdate )
						lprintf( WIDE( "(RECURSE UP!)Calling parent inline - wasn't clean, isn't clean, try and get it clean..." ) );
#endif
					// could adjust the clipping rectangle...
					DoUpdateCommonEx( upd, pc->parent, TRUE, level+1 DBG_RELAY );

					if( !upd->flags.bHasContent )
					{
                  //  control did not draw...
						//lprintf( WIDE( " Expected handling of this condition... Please return FALSE, and abort UpdateDIsplayPortion? Return now, leaving the rect without content?" ) );
						//DebugBreak();
					}
					if( !pc->flags.bParentCleaned || ( ( pc->parent && !pc->device )?pc->parent->flags.bDirty:0))
					{
						//DebugBreak();
						lprintf( WIDE( "Aborting my update... waiting for container to get his update done" ) );
						return;
					}

					if( pc->flags.bTransparent )
					{
						//lprintf( WIDE( "COPYING SURFACE HERE!?" ) );
						// we should be drawing when the parent does his thing...
						pc->OriginalSurface = CopyOriginalSurface( pc, pc->OriginalSurface );
					}
				}

				pc->draw_result = 0;
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					_lprintf(DBG_RELAY)( WIDE( " --- INVOKE DRAW (get region) --- %s " ), pc->pTypeName );
#endif
				{
#if LOCK_TEST
					PPHYSICAL_DEVICE device = GetFrame( pc )->device;
					if( device )
						LockRenderer( device->pActImg );
#endif
					// this causes a lock in that layer.?
					InvokeDrawMethod( pc, _DrawThySelf, ( pc ) );

#if LOCK_TEST
					if( device )
						UnlockRenderer( device->pActImg );
#endif
				}

#ifdef DEBUG_UPDAATE_DRAW
				// if it didn't draw... then why do anything?
				if( g.flags.bLogDebugUpdate )
					lprintf( WIDE( "draw result is... %d" ), pc->draw_result );
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
							lprintf( WIDE( "Parent is no longer cleaned...." ) );
#endif
						pc->flags.bCleanedRecently = 1;
						pc->flags.bParentCleaned = 0; // has now drawn itself, and we must assume that it's not clean.
						pc->flags.children_cleaned = 0;
					}

					//UnmakeImageFile( current);
				}


				// better clear this flag after so that a smudge during
				// a dumb control doesn't make us loop...
				// though I suppose some other control could cause us to draw again?
				// well leaving it set during will cause smudge to do what exactly


				pc->flags.bDirty = FALSE;
				//lprintf( WIDE("Invoked a draw self") );
				// the outermost border/frame will be drawn
				// from a different place... this one only needs to
				// worry aobut child region borders after telling them to
				// draw - to enforce cleanest bordering...
				if( g.flags.always_draw || pc->parent )
					if( pc->DrawBorder )  // and initial?
					{
#ifdef DEBUG_BORDER_DRAWING
						lprintf( WIDE( "Drawing border here too.." ) );
#endif
						pc->DrawBorder( pc );
					}

#if DEBUG_UPDAATE_DRAW > 2
				if( g.flags.bLogDebugUpdate )
				{
					if( upd->flags.bHasContent )
						_lprintf(DBG_RELAY)( WIDE("Added update region %d,%d %d,%d"), upd->x ,upd->y, upd->width, upd->height );
					else
						_lprintf(DBG_RELAY)( WIDE( "No prior content in update rect..." ) );
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
						_lprintf(DBG_RELAY)( WIDE("Added update region %d,%d %d,%d"), upd->x ,upd->y, upd->width, upd->height );
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
							lprintf( WIDE( "marking on child %p parent %p(%p) is %s;%s" ), child, pc, child->parent, cleaned?WIDE( "CLEANED" ):WIDE( "UNCLEAN" ), child->parent->flags.bDirty?WIDE( "DIRTY" ):WIDE( "not dirty" ) );
#endif
					}
				}
				//pc->flags.bDirty = 0;
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( WIDE("And now it has been cleaned...") );
#endif
			}
			else
			{
				//cleaned = 1; // well, lie here... cause we're already clean?
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( WIDE("Not invoking draw self.") );
#endif
				//#if 0
#if DEBUG_UPDAATE_DRAW > 3
				// general logging of the current status of the control
				// at this point the NORMAL status is clean, visible, no force, and drawing proc.
				if( g.flags.bLogDebugUpdate )
					lprintf( WIDE("control %p is %s %s %s %s"), pc, pc->flags.bDirty?WIDE( "dirty" ):WIDE( "clean" )
							 , bDraw?WIDE( "force" ):WIDE( "" )
							 , pc->flags.bHidden?WIDE( "hidden!" ):WIDE( "visible..." )
							 , pc->_DrawThySelf?WIDE( "drawingproc" ):WIDE( "NO DRAW PROC" ) );
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
					//lprintf( WIDE("doing a child update...") );
#if DEBUG_UPDAATE_DRAW > 2
					if( g.flags.bLogDebugUpdate )
						lprintf( WIDE( "updating child %p parent %p is %s;%s" ), child, pc, cleaned?WIDE( "CLEANED" ):WIDE( "UNCLEAN" ), child->parent->flags.bDirty?WIDE( "DIRTY" ):WIDE( "not dirty" ) );
#endif
					//lprintf( WIDE("Do update region %d,%d %d,%d"), upd->x ,upd->y, upd->width, upd->height );
					//child->flags.bParentCleaned = 1; // has now drawn itself, and we must assume that it's not clean.
					DoUpdateCommonEx( upd, child, cleaned, level+1 DBG_RELAY );
				}
			}
			if( pc->flags.bDirtied )
			{
				pc->flags.bDirtied = 0;
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( WIDE( "Recovered dirty that was set while we were cleaning... going back to draw again." ) );
#endif
				goto retry_update;
			}

			// after all children have updated, draw decorations (edit hotspots, cover animations...)
			InvokeMethod( pc, _DrawDecorations, ( pc ) );

#if LOCK_TEST
			if( device )
				UnlockRenderer( device->pActImg );
#endif

			pc->flags.children_cleaned = 1;
			pc->flags.bCleaning = 0;
			pc->flags.bFirstCleaning = 1;
		}
		else
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				_xlprintf(1 DBG_RELAY)( WIDE("Already Cleaning!!!!!  (THIS IS A FATAL LOCK POTENTIAL)") );
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
					lprintf( WIDE( "Parent is no longer cleaned..." ) );
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
							lprintf( WIDE("***doing a child update... to %s"), cleaned?WIDE( "CLEAN" ):WIDE( "UNCLEAN" ) );
#endif
						child->flags.bParentCleaned = cleaned; // has now drawn itself, and we must assume that it's not clean.
						//lprintf( WIDE("Do update region %d,%d %d,%d"), upd->x ,upd->y, upd->width, upd->height );
						//child->flags.bParentCleaned = 1; // has now drawn itself, and we must assume that it's not clean.
						DoUpdateCommonEx( upd, child, cleaned, level+1 DBG_SRC );
					}
				}
			}
#if DEBUG_UPDAATE_DRAW > 2
			if( g.flags.bLogDebugUpdate )
				_xlprintf(1 DBG_RELAY)( WIDE("Control %p already drawing itself!?"), pc );
#endif
		}
	}
#ifdef DEBUG_UPDAATE_DRAW
	else
		if( g.flags.bLogDebugUpdate )
			_xlprintf(1 DBG_RELAY)( WIDE("NULL control told to update!") );
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
	// if( pc->flags.bTransparent && pc->parent && !pc->parent->flags.bDirty )
	//    SmudgeCommon( pc->parent );
	if( g.flags.always_draw )
	{
		if( pc )
		{
			if( !pc->flags.bDestroy )
			{
				PSI_CONTROL frame = GetFrame( pc );
				PPHYSICAL_DEVICE device = frame?frame->device:NULL;
				if( device )
					MarkDisplayUpdated( device->pActImg );
			}
		}
		if( g.flags.bLogDebugUpdate )
			_lprintf(DBG_RELAY)( WIDE( "%p(%s) wanted to draw..." ), pc, pc->pTypeName );
		return;
	}
	else if( !g.flags.allow_threaded_draw )
	{
		PSI_CONTROL frame = GetFrame( pc );
		PPHYSICAL_DEVICE device = frame?frame->device:NULL;
		if( device )
		{
			AddLink( &device->pending_dirty_controls, pc );
			if( !g.flags.sent_redraw )
			{
				g.flags.sent_redraw = 1;
				Redraw( device->pActImg );
			}
		}
	}
	else if(pc)
	{
		if( pc->flags.bDestroy )
			return;
#if DEBUG_UPDAATE_DRAW > 0
		if( g.flags.bLogDebugUpdate )
			_lprintf(DBG_RELAY)( WIDE( "Smudge %p %s" ), pc, pc->pTypeName?pc->pTypeName:WIDE( "NoTypeName" ) );
#endif
		{
			PSI_CONTROL parent;
			for( parent = pc; parent; parent = parent->parent )
			{
				if( parent->flags.bNoUpdate || parent->flags.bHidden )
				{
#if DEBUG_UPDAATE_DRAW > 3
					if( g.flags.bLogDebugUpdate )
						lprintf( WIDE("a control %p(%d) (self, or some parent %p(%d)) has %s or %s  (marks me as dirty anhow, but doesn't attempt anything further)")
								 , pc, pc->nType, parent, parent->nType
								 , parent->flags.bNoUpdate?WIDE( "noupdate" ):WIDE( "..." )
								 , parent->flags.bHidden?WIDE( "hidden" ):WIDE( "..." )
								 );
#endif
					pc->flags.bDirty = 1;
					return;
				}
			}
		}
		if( pc->flags.bDirty || pc->flags.bCleaning )
		{
			if( pc->flags.bCleaning )
			{
				_32 tick = timeGetTime();
				// something changed, and we'll have to draw that control again... as soon as it's done cleaning actually.
				pc->flags.bDirtied = 1;
				while( pc->flags.bCleaning && pc->flags.bDirtied && ((tick+500) < timeGetTime() ) )
					WakeableSleep( 10 );
				if( !pc->flags.bCleaning )
					if( pc->flags.bDirtied )
						pc->flags.bDirty = 1;
			}
#if DEBUG_UPDAATE_DRAW > 3
			if( g.flags.bLogDebugUpdate )
				_xlprintf(LOG_DEBUG DBG_RELAY)( WIDE("%s %s %s %p")
														, pc->flags.bDirty?WIDE( "already smudged" ):WIDE( "" )
														,(pc->flags.bDirty && pc->flags.bCleaning)?WIDE( "and" ):WIDE( "" )
														, pc->flags.bCleaning?WIDE( "in process of cleaning..." ):WIDE( "" )
														, pc );
#endif

         //Sleep( 10 );
			//return;
		}
		else // if( !pc->flags.bDirty )
		{
			PSI_CONTROL parent;
#if DEBUG_UPDAATE_DRAW > 3
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE( "not dirty, and not cleaning" ) );
#endif

#ifdef DEBUG_UPDAATE_DRAW
			//if( pc->parent && pc->flags.bTransparent )
			//   SmudgeCommon( pc->parent );
			for( parent = pc; parent /*&& parent->flags.bTransparent*/ &&
				 !parent->InUse &&
				 !parent->flags.bDirty; parent = parent->parent )
			{
				if( g.flags.bLogDebugUpdate )
					_xlprintf( LOG_DEBUG DBG_RELAY ) ( WIDE("%s %p is %s and %s %s")
																, (parent==pc)?WIDE( "self" ):WIDE( "parent" )
																, parent
																, parent->InUse?WIDE( "USED" ):WIDE( "Not Used" )
																, parent->flags.bTransparent?WIDE( "Transparent" ):WIDE( "opaque" )
																, parent->flags.bChildDirty?WIDE( "has Dirty Child" ):WIDE( "children clean" ));
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
					_xlprintf(LOG_NOISE DBG_RELAY )( WIDE("marking myself dirty. %p"), pc );
			}
#endif
			pc->flags.bDirty = 1;
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
				lprintf( WIDE( "control is not in use... (smudge logic) Initial=%d " ), pc->flags.bInitial );
#endif
			if( !pc->flags.bInitial )
			{
				IntelligentFrameUpdateAllDirtyControls( pc DBG_RELAY );
			}
#ifdef DEBUG_UPDAATE_DRAW
			else
				if( g.flags.bLogDebugUpdate )
					lprintf( WIDE( "pc->flags.bInitial is true..." ) );
#endif
		}
	}
}

//---------------------------------------------------------------------------

void DoDumpFrameContents( int level, PSI_CONTROL pc )
{
	while( pc )
	{
		lprintf( WIDE("%*.*s") WIDE("Control %d(%d)%p at (%") _32fs WIDE(",%") _32fs WIDE(")-(%") _32f WIDE(",%") _32f WIDE(") (%") _32fs WIDE(",%") _32fs WIDE(")-(%") _32f WIDE(",%") _32f WIDE(") (%s %s %s %s) '%s' [%s]" )
		       , level*3,level*3,WIDE("----------------------------------------------------------------")
		       , pc->nID, pc->nType, pc
		       , pc->rect.x, pc->rect.y, pc->rect.width, pc->rect.height
		       , pc->surface_rect.x
		       , pc->surface_rect.y
		       , pc->surface_rect.width
		       , pc->surface_rect.height
		       , pc->flags.bTransparent?WIDE("transparent"):WIDE("t")
		       , pc->flags.bDirty?WIDE("dirty"):WIDE("d")
		       , pc->flags.bNoUpdate?WIDE("NoUpdate"):WIDE("nu")
		       , pc->flags.bHidden?WIDE("hidden"):WIDE("h")
		       , pc->caption.text?GetText(pc->caption.text):WIDE("")
		       , pc->pTypeName
		       );

		if( pc->child )
		{
			//lprintf( WIDE("%*.*s"), level*2,level*2,"                                                                                                          " );
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
	PENDING_RECT upd;
	upd.flags.bTmpRect = 0;
	upd.flags.bHasContent = 0;

	if( !(pc->flags.bNoUpdate || pc->flags.bHidden) )
	{
		PSI_CONTROL frame = GetFrame( pc );
		if( frame ) // else we're being destroyed probably...
		{
#if DEBUG_UPDAATE_DRAW > 2
			if( g.flags.bLogDebugUpdate )
				_xlprintf(LOG_NOISE DBG_RELAY )( WIDE( "Updating common ( which invokes frame... )%p %p" ), pc, frame );
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
					lprintf( WIDE("Updated all commons? flush display %d,%d  %d,%d"), upd.x,upd.y, upd.width, upd.height );
#endif
				TESTCOLOR=SetAlpha( BASE_COLOR_GREEN, 0x80 );
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
static int OnCreateCommon( WIDE("Frame") )( PSI_CONTROL pc )
{
	// cannot fail create frame - it's a simple control
	// later - if DisplayFrame( pc ) creates a physical display
	// for such a thing, and DIsplayFrameOver if that should be
	// modally related to another contorl/frame
	return 1;
}

void CPROC DeinitFrame( PSI_CONTROL pc )
{
   PPHYSICAL_DEVICE pf = pc->device;
	if( pf )
	{
	}
}


CONTROL_REGISTRATION frame_controls = { WIDE("Frame"), { { 320, 240 }, 0, BORDER_NORMAL }
};

PRIORITY_PRELOAD( register_frame_control, PSI_PRELOAD_PRIORITY )
{
   DoRegisterControl( &frame_controls, sizeof( frame_controls ) );
}

//---------------------------------------------------------------------------
static void LinkInNewControl( PSI_CONTROL parent, PSI_CONTROL elder, PSI_CONTROL child );

PROCEDURE RealCreateCommonExx( PSI_CONTROL *pResult
									  , PSI_CONTROL pContainer
										// if not typename, must pass type ID...
									  , CTEXTSTR pTypeName
									  , _32 nType
										// position of control
									  , int x, int y
									  , int w, int h
									  , _32 nID
									  , CTEXTSTR pIDName
										// ALL controls have a caption...
									  , CTEXTSTR text
										// fields in this override the defaults...
										// if not 0.
									  , _32 ExtraBorderType
									  , int bCreate // if !bCreate, return reload type
										// if this is reloaded...
										//, PTEXT parameters
										//, va_list args
										DBG_PASS )
{
	TEXTCHAR mydef[256];
	PCLASSROOT root;
	PSI_CONTROL pc = NULL;
	_32 BorderType;

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
		snprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY WIDE("/control/%s"), pTypeName );
	else
		snprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY WIDE("/control/%") _32f , nType );
	root = GetClassRoot( mydef );
	if( pTypeName )
		nType = (int)(long)GetRegisteredValueExx( root, NULL, WIDE("type"), TRUE );
	else
		pTypeName = GetRegisteredValueExx( root, NULL, WIDE("type"), FALSE );

	pc = (PSI_CONTROL)AllocateEx( sizeof( FR_CT_COMMON ) DBG_RELAY );
	MemSet( pc, 0, sizeof( FR_CT_COMMON ) );
	pc->class_root = root;
	pc->basecolors = basecolor( pContainer );
	{
		_32 size = GetRegisteredIntValue( root, WIDE("extra") );
		if( size )
		{
			POINTER data = Allocate( size );
			if( data )
				MemSet( data, 0, size );
			SetControlData( POINTER, pc, data );
		}
	}
	if( !w )
		w = GetRegisteredIntValue( root, WIDE("width") );
	if( !h )
		h = GetRegisteredIntValue( root, WIDE("height") );
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
	BorderType = (int)(long)GetRegisteredValueExx( root, NULL, WIDE("border"), TRUE );
	BorderType |= ExtraBorderType;
	//lprintf( WIDE("BorderType is %08x"), BorderType );
	if( !(BorderType & BORDER_FIXED) )
	{
		PSI_CONTROL _pc;
		for( _pc = pContainer;_pc;_pc=_pc->parent )
			if( _pc->flags.bScaled
				&& !(_pc->BorderType & BORDER_FIXED ) )
			{
				x = ScaleValue( &_pc->scalex, x );
				w = ScaleValue( &_pc->scalex, w );
				y = ScaleValue( &_pc->scaley, y );
				h = ScaleValue( &_pc->scaley, h );
				pc->flags.bScaled = 1; // its own scaler will be 1:1 since it's alredy scaled...
				break;
			}
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

	//else
	//	pc->caption.font = g.default_font;

	// from here forward, root and mydef reference the RTTI of the control...
	//
	snprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY WIDE("/control/%") _32f WIDE("/rtti"), nType );
	root = GetClassRoot( mydef );
	SetCommonDraw( pc, GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,int,WIDE("draw"),(PSI_CONTROL)));
	SetCommonDrawDecorations( pc, GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,WIDE("decoration_draw"),(PSI_CONTROL)));
	SetCommonMouse( pc, GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,int,WIDE("mouse"),(PSI_CONTROL,S_32,S_32,_32)));
	SetCommonKey( pc, GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,int,WIDE("key"),(PSI_CONTROL,_32)));
	pc->Destroy        = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,WIDE("destroy"),(PSI_CONTROL));
	pc->CaptionChanged = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,WIDE("caption_changed"),(PSI_CONTROL));
	pc->ChangeFocus    = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,WIDE("focus_changed"),(PSI_CONTROL,LOGICAL));
	pc->AddedControl   = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,WIDE("add_control"),(PSI_CONTROL,PSI_CONTROL));
	pc->Resize         = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,WIDE("resize"),(PSI_CONTROL,LOGICAL));
	pc->Rescale        = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,WIDE("rescale"),(PSI_CONTROL));
	//pc->PosChanging    = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,WIDE("position_changing"),(PSI_CONTROL,LOGICAL));
	pc->BeginEdit      = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,WIDE("begin_frame_edit"),(PSI_CONTROL));
	pc->EndEdit        = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,void,WIDE("end_frame_edit"),(PSI_CONTROL));
	// creates
	pc->flags.bInitial = 1;
	pc->flags.bDirty = 1;
	if( pc->nType == CONTROL_FRAME && g.default_font )
	{
		SetCommonFont( pc, g.default_font );
	}
	SetCommonBorder( pc, BorderType );
	pc->flags.bSetBorderType = 0;

	SetCommonText( pc, text );
	if( pContainer )
	{
		LinkInNewControl( pContainer, NULL, pc );
		pContainer->InUse++;
		//lprintf( WIDE("Added one to use..") );
		if( !pContainer->flags.bDirty )
			SmudgeCommon( pc );
		pContainer->InUse--;
		//lprintf( WIDE("Removed one to use..") );
	}

	if( pResult )
		*pResult = pc;

	if( !bCreate )
	{
		int (CPROC*Restore)(PSI_CONTROL,PTEXT);
		// allow init to return FALSE to destroy the control...
		Restore = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,int,WIDE("load"),(PSI_CONTROL,PTEXT));
		if( Restore )
			return (PROCEDURE)Restore;
	}
	else
	{
		int (CPROC*Init)(PSI_CONTROL,va_list);
		// allow init to return FALSE to destroy the control...
		Init = GetRegisteredProcedureExx(root,(CTEXTSTR)NULL,int,WIDE("init"),(PSI_CONTROL,va_list));
		if( Init )
			return (PROCEDURE)Init;
	}
	return NULL; //pc;
}

//---------------------------------------------------------------------------

#define CreateCommonExx(pc,pt,nt,x,y,w,h,id,cap,ebt,p,ep) CreateCommonExxx(pc,pt,nt,x,y,w,h,id,NULL,cap,ebt,p,ep)
PSI_PROC( PSI_CONTROL, CreateCommonExxx)( PSI_CONTROL pContainer
													 , CTEXTSTR pTypeName
													 , _32 nType
													 , int x, int y
													 , int w, int h
													 , _32 nID
													 , CTEXTSTR pIDName
													 , CTEXTSTR caption
													 , _32 ExtraBorderType
													 , PTEXT parameters
													 , POINTER extra_param
													  DBG_PASS );

//---------------------------------------------------------------------------

PSI_PROC( PSI_CONTROL, CreateFrame )( CTEXTSTR caption
										  , int x, int y
										  , int w, int h
										  , _32 BorderTypeFlags
										  , PSI_CONTROL hAbove )
{
	PSI_CONTROL pc;
	//lprintf( WIDE("Creating a frame at %d,%d %d,%d"), x, y, w, h );
#ifdef USE_INTERFACES
	GetMyInterface(); // macro with builtin quickcheck
#endif
	pc = CreateCommonExx( (BorderTypeFlags & BORDER_WITHIN)?hAbove:NULL
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
	SetCommonBorder( pc, BorderTypeFlags|((BorderTypeFlags & BORDER_WITHIN)?0:BORDER_FRAME) );
	//lprintf( WIDE("FRAME is %p"), pc );
	return pc;
}

//---------------------------------------------------------------------------

PSI_PROC( PTRSZVAL, GetCommonUserData )( PSI_CONTROL pf )
{
	if( pf )
		return pf->psvUser;
	return 0;
}

//---------------------------------------------------------------------------

PSI_PROC( void, SetCommonUserData )( PSI_CONTROL pf, PTRSZVAL psv )
{
	if( pf )
		pf->psvUser = psv;
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
			//lprintf( WIDE( "(%p)Parent h %d i %d" ), parent, parent_hidden, parent_initial );
		}
		//lprintf( WIDE( "Parent h %d i %d" ), parent_hidden, parent_initial );
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			_xlprintf(LOG_NOISE DBG_RELAY)( WIDE("Revealing %p %s"), pc, pc->pTypeName?pc->pTypeName:WIDE( "NoTypeName" ) );
#endif
		if( pc->device )
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE("showing a renderer...") );
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
					lprintf( WIDE("Found something hidden... revealing it, and all children.") );
#endif
				for( child = pc->child; child; child = child->next )
					if( !child->flags.bHiddenParent )
						RevealCommon( child );
			}
#ifdef DEBUG_UPDAATE_DRAW
			else
				if( g.flags.bLogDebugUpdate )
					lprintf( WIDE( "Control was hidden, now showing." ) );
#endif
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
				lprintf( WIDE( "Initial, no draw" ) );
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
			lprintf( WIDE("-------- OPEN DEVICE") );
#endif
		pc->flags.bInitial = FALSE;
		pf = OpenPhysicalDevice( pc, over, pActImg, under );
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			lprintf( WIDE("-------- device opened") );
#endif
	}
#ifdef DEBUG_CREATE
	lprintf( WIDE("-------------------- Display frame has been invoked...") );
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
				lprintf( WIDE( "********* FIrst showing of this window - it's been bINitial for so long..." ) );
#endif
			// one final draw to make sure everyone is clean.
			//lprintf( WIDE("Dipsatch draw.") );
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
				lprintf( WIDE( "inital was set to false...." ) );
			SetDrawBorder( pc ); // re-sets routines, but also calls draws for border and caption.

			pc->flags.bHidden = TRUE; // fake this.. so reveal shows it... hidden parents will remain hidden
		}
		RevealCommon( pc );
		//lprintf( WIDE("Draw common?!") );
		/* wait for the draw event from the video callback... */
		//UpdateCommonEx( pc, FALSE DBG_SRC );
		//SyncRender( pf->pActImg );
		if( pc->flags.bEditSet )
			EditFrame( pc, !pc->flags.bNoEdit );
	}

}

PSI_PROC( void, DisplayFrameOverOn )( PSI_CONTROL pc, PSI_CONTROL over, PRENDERER pActImg )
{
	DisplayFrameOverOnUnder( pc, over, pActImg, NULL );
}


//---------------------------------------------------------------------------

PSI_PROC( void, DisplayFrameOver )( PSI_CONTROL pc, PSI_CONTROL over )
{
	//lprintf( WIDE("Displayframeover") );
	DisplayFrameOverOn( pc, over, NULL );
	//SmudgeCommon( pc );
}

//---------------------------------------------------------------------------

PSI_PROC( void, DisplayFrameUnder )( PSI_CONTROL pc, PSI_CONTROL under )
{
	//lprintf( WIDE("Displayframeover") );
	DisplayFrameOverOnUnder( pc, NULL, NULL, under );
	SmudgeCommon( pc );
}

//---------------------------------------------------------------------------

PSI_PROC( void, DisplayFrame )( PSI_CONTROL pc )
{
	//lprintf( WIDE("DisplayFrame") );
	DisplayFrameOverOn( pc, NULL, NULL );
}

//---------------------------------------------------------------------------

PSI_PROC( void, DisplayFrameOn )( PSI_CONTROL pc, PRENDERER pActImg )
{
	//lprintf( WIDE("DisplayfFrameOn") );
	DisplayFrameOverOn( pc, NULL, pActImg );
}

//---------------------------------------------------------------------------

PSI_PROC( void, HideCommon )( PSI_CONTROL pc )
{
	/* should additionally wrap this with a critical section */
	static int levels;
	int hidden = 0;
	if( !pc )
		return;
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		lprintf( WIDE( "Hide common %p %s" ), pc, pc->pTypeName?pc->pTypeName:WIDE( "NoTypeName" ) );
#endif
	//PSI_CONTROL _pc = pc;
	levels++;
	if( levels == 1 )
		pc->flags.bHiddenParent = 1;
	if( pc->flags.bHidden ) // already hidden, forget someeone telling me this.
	{
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			lprintf( WIDE( "Already hidden..." ) );
#endif
		levels--;
		return;
	}
	pc->flags.bHidden = 1;
	if( pc->flags.bInitial )
	{
#ifdef DEBUG_UPDAATE_DRAW
		if( g.flags.bLogDebugUpdate )
			lprintf( WIDE( "Control hasn't been shown yet..." ) );
#endif
		// even if not shown, do mark this hidden control as a hidden parent
		// so it's not auto unhidden on revealing the containing frame.
		levels--;
		return;
	}
	//lprintf( WIDE("HIDING %p(%d)"), pc, pc->nType );
	if( pc->device )
	{
		//lprintf( WIDE("hiding a display image...") );
		pc->flags.bNoUpdate = 1;
		HideDisplay( pc->device->pActImg );
	}
	if( pc )
	{
		PSI_CONTROL child;
		hidden = 1;
		for( child = pc->child; child; child = child->next )
		{
			/* hide all children, which will trigger /dev/null update */
			HideCommon( child );
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
			//lprintf( WIDE("Added one to use..") );
			if( pc->flags.bTransparent && pc->OriginalSurface )
			{
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( WIDE( "transparent thing..." ) );
#endif
				if( !pc->flags.bParentCleaned && pc->OriginalSurface )
				{
#ifdef DEBUG_UPDAATE_DRAW
					if( g.flags.bLogDebugUpdate )
						lprintf( WIDE( "Restoring old surface..." ) );
					if( g.flags.bLogDebugUpdate )
						lprintf( WIDE( "Restoring orignal background... " ) );
#endif
					BlotImage( pc->Window, pc->OriginalSurface, 0, 0 );
					pc->flags.bParentCleaned = 1;
					{
						PENDING_RECT upd;
#ifdef DEBUG_UPDAATE_DRAW
						if( g.flags.bLogDebugUpdate )
							lprintf( WIDE("pc = %p par = %p"), pc, pc->parent );
#endif
						upd.flags.bHasContent = 0;
						upd.flags.bTmpRect = 0;
						//lprintf( WIDE("doing update common...") );
#ifdef DEBUG_UPDAATE_DRAW
						if( g.flags.bLogDebugUpdate )
							lprintf( WIDE( "Flushing this button to the display..." ) );
#endif
						TESTCOLOR=SetAlpha( BASE_COLOR_YELLOW, 0x20 );
						DoUpdateFrame( pc
										 , pc->rect.x, pc->rect.y
										 , pc->rect.width, pc->rect.height
										 , TRUE
										  DBG_SRC );
					}
				}
				else if( pc->OriginalSurface )
				{
#ifdef DEBUG_UPDAATE_DRAW
					if( g.flags.bLogDebugUpdate )
						lprintf( WIDE( "Parent was already clean... did nothing" ) );
#endif
				}
				else
				{
					lprintf( WIDE("NOthing to recover?") );
				}
			}
			else
			{
#ifdef DEBUG_UPDAATE_DRAW
				if( g.flags.bLogDebugUpdate )
					lprintf( WIDE("Parent needs to be drawn, no auto recovery available" ));
#endif
				SmudgeCommon( pc->parent );
			}
		}
#ifdef DEBUG_UPDAATE_DRAW
		else
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE( "..." ) );
#endif
	}
#ifdef DEBUG_UPDAATE_DRAW
	if( g.flags.bLogDebugUpdate )
		lprintf( WIDE( "levels: %d" ), levels );
#endif
}

//---------------------------------------------------------------------------

PSI_PROC( void, SizeCommon )( PSI_CONTROL pc, _32 width, _32 height )
{
	if( pc && pc->Resize )
		pc->Resize( pc, TRUE );
	//if( pc->nType )
	{
		PPHYSICAL_DEVICE pFrame = GetFrame(pc)->device;
		//ValidatedControlData( PFRAME, CONTROL_FRAME, pFrame, GetFrame( pc ) );
		IMAGE_RECTANGLE old;
		PEDIT_STATE pEditState;
		S_32 delw, delh;

		pc->original_rect.width = width;
		pc->original_rect.height = height;
		if( !pc->parent && pc->device && pc->device->pActImg )
		{
			// we now have this accuragely handled.
			//  rect is active (with frame)
			//  oroginal_rect is the size it was before having to extend it
			// somwehere between original_rect changes and rect changes surface_rect is recomputed
			//lprintf( WIDE("Enlarging size...") );
			width += FrameBorderX(pc, pc->BorderType);
			height += FrameBorderY(pc, pc->BorderType, GetText( pc->caption.text ) );
			SizeDisplay( pc->device->pActImg, width, height );

		}
		delw = (S_32)width - (S_32)pc->rect.width;
		delh = (S_32)height - (S_32)pc->rect.height;

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

		{
			extern void UpdateSurface( PSI_CONTROL pc );
			UpdateSurface( pc );
		}

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

PSI_PROC( void, SizeCommonRel )( PSI_CONTROL pc, _32 w, _32 h )
{
	SizeCommon( pc, w + pc->rect.width, h + pc->rect.height );
}

//---------------------------------------------------------------------------

void InvokePosChange( PSI_CONTROL pc, LOGICAL updating )
{
	void (CPROC *OnChanging)(PSI_CONTROL,LOGICAL);
	PCLASSROOT data = NULL;
	PCLASSROOT event_root = GetClassRootEx( pc->class_root, WIDE( "position_changing" ) );
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
	PCLASSROOT event_root = GetClassRootEx( pc->class_root, WIDE( "some_parents_position_changing" ) );
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


PSI_PROC( void, MoveCommon )( PSI_CONTROL pc, S_32 x, S_32 y )
{
	if( pc )
	{
		PPHYSICAL_DEVICE pf = pc->device;
		InvokePosChange( pc, TRUE );
		pc->original_rect.x = x;
		pc->original_rect.y = y;
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

void ScaleCoords( PSI_CONTROL pc, PS_32 a, PS_32 b )
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
	while( pc && !pc->flags.bScaled )
		pc = pc->parent;
	if( pc )
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
		MoveSizeCommon( pc
						  , ScaleValue( &pc->scalex, pc->original_rect.x )
						  , ScaleValue( &pc->scaley, pc->original_rect.y )
						  , ScaleValue( &pc->scalex, pc->original_rect.width )
						  , ScaleValue( &pc->scaley, pc->original_rect.height )
						  );
		if( pc && pc->Rescale )
			pc->Rescale( pc );
		ApplyRescaleChild( pc, &pc->scalex, &pc->scaley );
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

void SetCommonFont( PSI_CONTROL pc, SFTFont font )
{
	if( pc )
	{
		_32 w, h;
		_32 _w, _h;
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
			GetStringSizeFont( WIDE("XXXXX"), &_w, &_h, NULL/*pc->caption.font*/ );
			GetStringSizeFont( WIDE("XXXXX"), &w, &h, font );
			if(h == 0 )
				DebugBreak();

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
				pc->flags.bScaled = 0;
			}
			if( pc && pc->Rescale )
				pc->Rescale( pc );
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
			if( !pc->child )
				ApplyRescale( pc );
		}
		//else
		//	lprintf( "not setting scaling..." );

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
	//_xlprintf(1 DBG_RELAY )( WIDE("Someone getting font from %p"), pc );
	while( pc )
	{
		// lprintf( WIDE("Checking control %p for font %p"), pc, pc->caption.font );
		if( pc->caption.font )
			return pc->caption.font;
		if( pc->device ) // devices end parent relation also... we maintain (for some reason) parent link to parent control....
			break;
		pc = pc->parent;
	}
	//lprintf( WIDE("No control, no font.") );
	// results in DefaultFont() when used anyhow...
	return NULL;
}

//---------------------------------------------------------------------------

PSI_PROC( void, MoveCommonRel )( PSI_CONTROL pc, S_32 x, S_32 y )
{
   MoveFrame( pc, x + pc->rect.x, y + pc->rect.y );
}

//---------------------------------------------------------------------------

PSI_PROC( void, MoveSizeCommon )( PSI_CONTROL pc, S_32 x, S_32 y, _32 width, _32 height )
{
	if( pc )
	{
		PPHYSICAL_DEVICE pf = pc->device;
		PEDIT_STATE pEditState = pf?&pf->EditState:NULL;
		IMAGE_RECTANGLE old;
		// timestamp these...
		//lprintf( WIDE("move %p %d,%d %d,%d"), pc, x, y, width, height );
		if( !pc )
			return;
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

PSI_PROC( void, MoveSizeCommonRel )( PSI_CONTROL pc, S_32 x, S_32 y, _32 width, _32 height )
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
		if( pc->nID == ID )
			break;
		pc = pc->next;
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
				lprintf( WIDE( "Enable Common Updates on %p" ), common );
#endif
			common->flags.bNoUpdate = FALSE;
			// probably doing mass updates so just mark the status, and make the application draw.
		}
		else
		{
#ifdef DEBUG_UPDAATE_DRAW
			if( g.flags.bLogDebugUpdate )
				lprintf( WIDE( "Disable Common Updates on %p" ), common );
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
	//lprintf( WIDE("GetText was %d"), buflen );
	buffer[buflen-1] = 0; // make sure we nul terminate..
	if( bCString ) // use C processing on escapes....
	{
		int n, ofs, escape = 0;
		ofs = 0;
		//lprintf( WIDE("GetText was %d"), buflen );
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

PSI_PROC( void, SetCommonText )( PSI_CONTROL pc, CTEXTSTR text )
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
		//lprintf( WIDE("invoke caption changed... ") );
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
		//lprintf( WIDE("Control draw %p"), pc );
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
					//lprintf( WIDE("skipping %p..."), pAdd );
				pAdd = pAdd->next;
			}
			if( !pAdd )
			{
				//lprintf( WIDE("Adding control first...") );
				child->prior = NULL;
				parent->child = child;
			}
			else
			{
				//lprintf( WIDE("Adding control after last...") );
				child->prior = pAdd;
				pAdd->next = child;
			}
			child->next = NULL;
		}
		child->parent = parent;
	}
}

//---------------------------------------------------------------------------

void GetControlSize( PSI_CONTROL _pc, P_32 w, P_32 h )
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
								, _32 nType
								 // position of control
								, int x, int y
								, int w, int h
								, _32 nID
								, CTEXTSTR pIDName // if this is NOT NULL, use Named ID to ID the control.
								// ALL controls have a caption...
							  , CTEXTSTR text
								// fields in this override the defaults...
								// if not 0.
							  , _32 ExtraBorderType
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
		if( !((int(CPROC *)(PSI_CONTROL,POINTER))proc)( pResult, extra_param ) )
		{
			_xlprintf(1 DBG_RELAY )( WIDE("Failed to init the control - destroying it.") );
			DestroyCommon( &pResult );
		}
	}
	if( pResult ) // no point in doing anything extra if the initial init fails.
		{
			TEXTCHAR mydef[256];
			if( pTypeName )
				snprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY WIDE("/control/%s/rtti/extra init"), pTypeName );
			else
				snprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY WIDE("/control/%") _32f WIDE("/rtti/extra init"), nType );
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
					for( name = GetFirstRegisteredName( WIDE("psi/control/rtti/extra init"), &data );
						 name;
						  name = GetNextRegisteredName( &data ) )
					{
						CustomInit = GetRegisteredProcedureExx((PCLASSROOT)data,(CTEXTSTR)NULL,int,name,(PSI_CONTROL));
						if( CustomInit )
						{
							if( !CustomInit( pResult ) )
							{
								lprintf( WIDE("extra init has returned failure... so what?") );
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
							lprintf( WIDE("extra init has returned failure... so what?") );
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
					, _32 nType
					, int x, int y
					, int w, int h
					, _32 nID
					//, ...
					)
{
	return CreateCommonExx( pFrame, NULL, nType, x, y, w, h, nID, NULL, 0, NULL, NULL DBG_SRC );
}

PSI_CONTROL MakeControlParam( PSI_CONTROL pFrame
								, _32 nType
								, int x, int y
								, int w, int h
								, _32 nID
								, POINTER parameter
								)
{
   return CreateCommonExx( pFrame, NULL, nType, x, y, w, h, nID, NULL, 0, NULL, parameter DBG_SRC );
}

PSI_CONTROL MakePrivateControl( PSI_CONTROL pFrame
								  , _32 nType
								  , int x, int y
								  , int w, int h
								  , _32 nID
								  )
{
	return CreateCommonExx( pFrame, NULL, nType
								 , x, y
								 , w, h
								 , nID, NULL
								 , BORDER_NO_EXTRA_INIT
								 , NULL, NULL DBG_SRC );
}

PSI_CONTROL MakePrivateNamedControl( PSI_CONTROL pFrame
								  , CTEXTSTR pType
								  , int x, int y
								  , int w, int h
								  , _32 nID
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
									 , _32 nType
									 , int x, int y
									 , int w, int h
									 , _32 nID
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
																		  , _32 nID
																		  , CTEXTSTR caption
																		  )
{
	return CreateCommonExxx( pContainer, pType, 0, x, y, w, h, nID, pIDName, caption, 0, NULL, NULL DBG_SRC );
}

PSI_PROC( PSI_CONTROL, MakeNamedCaptionedControl )( PSI_CONTROL pContainer
															 , CTEXTSTR pType
															 , int x, int y
															 , int w, int h
															 , _32 nID
															 , CTEXTSTR caption
															 )
{
	return CreateCommonExx( pContainer, pType, 0, x, y, w, h, nID, caption, 0, NULL, NULL DBG_SRC );
}


PSI_CONTROL VMakeCaptionedControl( PSI_CONTROL pFrame
									 , _32 nType
									 , int x, int y
									 , int w, int h
									 , _32 nID
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
						  , _32 nID
								//, ...
								)
{
	//va_list args;
   //va_start( args, nID );
   return CreateCommonExx( pFrame, pType, 0, x, y, w, h, nID, NULL, 0, NULL, NULL DBG_SRC );
}

PSI_CONTROL VMakeControl( PSI_CONTROL pFrame
                        , _32 nType
                        , int x, int y
                        , int w, int h
                        , _32 nID
                        )
{
   return CreateCommonExx( pFrame, NULL, nType, x, y, w, h, nID, NULL, 0, NULL, NULL DBG_SRC );
}

PSI_CONTROL RestoreControl( PSI_CONTROL pFrame
                          , _32 nType
                          , int x, int y
                          , int w, int h
                          , _32 nID
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
		if( !pc->InUse && !pc->InWait )
		{
			level++;
			AddUse( pc );
			//lprintf( WIDE("Destroying control %p"), pc );
			while( pc->child )
			{
				//lprintf( WIDE("destroying child control %p"), pc->child );
				DestroyCommonExx( &pc->child, level DBG_RELAY );
			}
			if( pc->caption.text )
			{
				//lprintf( WIDE("Release caption text") );
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
					snprintf( mydef, sizeof( mydef ), WIDE("psi/control/rtti/extra destroy") );
					for( name = GetFirstRegisteredName( mydef, &data );
						 name;
						  name = GetNextRegisteredName( &data ) )
					{
						CustomDestroy = GetRegisteredProcedureExx((PCLASSROOT)data,(CTEXTSTR)NULL,int,name,(PSI_CONTROL));
						if( CustomDestroy )
						{
							lprintf( WIDE("Invoking custom destroy") );
							if( !CustomDestroy( pc ) )
							{
								lprintf( WIDE("extra destroy has returned failure... so what?") );
							}
						}
					}

					snprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY WIDE("/control/%d/rtti/extra destroy"), pc->nType );
					for( name = GetFirstRegisteredName( mydef, &data );
						 name;
						  name = GetNextRegisteredName( &data ) )
					{
						CustomDestroy = GetRegisteredProcedureExx((PCLASSROOT)data,(CTEXTSTR)NULL,int,name,(PSI_CONTROL));
						if( CustomDestroy )
						{
							if( !CustomDestroy( pc ) )
							{
								lprintf( WIDE("extra destroy has returned failure... so what?") );
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
						//   lprintf( WIDE("Current is not %p it is %p"), pc, pf->pCurrent );
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
								lprintf( WIDE("update some controls is a edit thing...") );
								SmudgeSomeControls( pf->common, &upd );
							}
						}
					}
					//else
					//   lprintf( WIDE("no device which might have a current...") );
				}
				//else
				//   lprintf( WIDE("no frame to unmake current") );
			}
			if( pc->device ) // only thing which may have a commonwait
			{
				PCOMMON_BUTTON_DATA pcbd = &pc->parent->pCommonButtonData;
				if( pcbd )
				{
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


BUTTON_CLICK( ButtonOkay, ( PTRSZVAL psv, PSI_CONTROL pc ) )
{
	PCOMMON_BUTTON_DATA pcbd = pc->parent?&pc->parent->pCommonButtonData:NULL;
	{
		int *val = (int*)psv;
		if( val )
			*val = TRUE;
		else
			if( pcbd->done_value )
				(*pcbd->done_value) = TRUE;
	}
	if( pcbd->thread )
		WakeThread( pcbd->thread );
}

//---------------------------------------------------------------------------

PSI_PROC( void, InitCommonButton )( PSI_CONTROL pc, int *value )
{
   //ConfigButton( pc, NULL, ButtonOkay, (PTRSZVAL)value );
}

//---------------------------------------------------------------------------

void SetCommonButtons( PSI_CONTROL pf
                     , int *pdone
                     , int *pokay )
{
	if( pf )
	{
		PCOMMON_BUTTON_DATA pcbd;
		SetButtonPushMethod( GetControl( pf, BTN_CANCEL ), ButtonOkay, (PTRSZVAL)pdone );
		SetButtonPushMethod( GetControl( pf, BTN_OKAY ), ButtonOkay, (PTRSZVAL)pokay );
		pcbd = &pf->pCommonButtonData;
		pcbd->okay_value = pokay;
		pcbd->done_value = pdone;
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
		PCOMMON_BUTTON_DATA pcbd;
		PCONTROL pc;
		int x, x2;
		int y;
		//  lprintf( WIDE("Buttons will be added at... %d, %d")
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
								, BTN_OKAY, okaytext, 0, ButtonOkay, (PTRSZVAL)okay );
			//SetCommonUserData( pc, (PTRSZVAL)pcbd );
		}
		if( done && donetext )
		{
			pc = MakeButton( pf
						  , x2, y
						  , COMMON_BUTTON_WIDTH, COMMON_BUTTON_HEIGHT
						  , BTN_CANCEL, donetext, 0, ButtonOkay, (PTRSZVAL)done );
			//SetCommonUserData( pc, (PTRSZVAL)pcbd );
		}
	}
}

//---------------------------------------------------------------------------

void AddCommonButtons( PSI_CONTROL pf, int *done, int *okay )
{
	AddCommonButtonsEx( pf, done, WIDE("Cancel"), okay, WIDE("OK") );
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
		if( (!pc->parent || (pc->device)) && !pc->flags.bDestroy )
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
	PCOMMON_BUTTON_DATA pcbd;
	//AddWait( pc );
	pcbd = New( COMMON_BUTTON_DATA );
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
			//lprintf( WIDE("Sleeping forever, cause I'm not doing anything else...") );
			WakeableSleep( SLEEP_FOREVER );
		}
}

//---------------------------------------------------------------------------
PSI_PROC( void, CommonWaitEndEdit)( PSI_CONTROL *pf ) // a frame in edit mode, once edit mode done, continue
{
	PCOMMON_BUTTON_DATA pcbd;
	AddWait( (*pf) );
	pcbd = &(*pf)->pCommonButtonData;
	pcbd->thread = MakeThread();
	pcbd->flags.bWaitOnEdit = 1;
	while( !(*pf)->flags.bDestroy
			&& ( pcbd->flags.bWaitOnEdit )
		  )
		if( !Idle() )
		{
			//lprintf( WIDE("Sleeping forever, cause I'm not doing anything else..>") );
			WakeableSleep( SLEEP_FOREVER );
		}
	DeleteWaitEx( pf DBG_SRC );
}

PSI_PROC( void, CommonWait)( PSI_CONTROL pc ) // perhaps give a callback for within the loop?
{
	if( pc )
	{
		PCOMMON_BUTTON_DATA pcbd;
		AddWait( pc );
		pcbd = &pc->pCommonButtonData;
		pcbd->thread = MakeThread();
		pcbd->flags.bWaitOnEdit = 0;
		while( !pc->flags.bDestroy
				&& !( ( pcbd->okay_value )?( *pcbd->okay_value ):0 )
				&& !( ( pcbd->done_value )?( *pcbd->done_value ):0 )
			  )
			if( !Idle() )
			{
				//lprintf( WIDE("Sleeping forever, cause I'm not doing anything else..>") );
				WakeableSleep( SLEEP_FOREVER );
			}
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
		//lprintf( WIDE("Failing adopt: %p %p %p %d")
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
													, pOrphan->original_rect.x, pOrphan->original_rect.y
													, pOrphan->original_rect.width, pOrphan->original_rect.height
													);
	}
	if( pOrphan->Surface )
		AdoptSubImage( pOrphan->Window, pOrphan->Surface );
	else
	{
		lprintf( WIDE("!!!!!!!! No Surface on control!?") );
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

PCONTROL CreateControlExx( PSI_CONTROL pFrame
								  , _32 attr
								  , int x, int y
								  , int w, int h
								  , int nID
								  , int BorderType
								  , int extra
								  , ControlInitProc InitProc
								  , PTRSZVAL psvInit
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

void GetPhysicalCoordinate( PSI_CONTROL relative_to, S_32 *_x, S_32 *_y, int include_surface )
{
	S_32 x = (*_x);
	S_32 y = (*_y);
	while( relative_to )
	{
		if( include_surface )
			x += relative_to->surface_rect.x;
		x += relative_to->rect.x;
		if( include_surface )
			y += relative_to->surface_rect.y;
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
	PPHYSICAL_DEVICE pf = pcf->device;
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
									  , int (CPROC*MouseMethod)(PSI_CONTROL, S_32 x, S_32 y, _32 b ) )
{
	if( MouseMethod )
	{
		pc->_MouseMethod = (__MouseMethod*)Reallocate( pc->_MouseMethod, ( pc->n_MouseMethod + 1 ) * sizeof( pc->_MouseMethod[0] ) );
		pc->_MouseMethod[pc->n_MouseMethod++] = MouseMethod;
	}
}

//---------------------------------------------------------------------------

void SetCommonMouse( PSI_CONTROL pc
									  , int (CPROC*MouseMethod)(PSI_CONTROL, S_32 x, S_32 y, _32 b ) )
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
									 ,int (CPROC*Key)(PSI_CONTROL,_32) )
{
	if( Key )
	{
		pc->_KeyProc = (__KeyProc*)Reallocate( pc->_KeyProc, ( pc->n_KeyProc + 1 ) * sizeof( pc->_KeyProc[0] ) );
		pc->_KeyProc[pc->n_KeyProc++] = Key;
	}
}

//---------------------------------------------------------------------------

void SetCommonKey( PSI_CONTROL pc
									 ,int (CPROC*Key)(PSI_CONTROL,_32) )
{
	if( Key )
	{
		pc->_KeyProc = (__KeyProc*)Preallocate( pc->_KeyProc, ( pc->n_KeyProc + 1 ) * sizeof( pc->_KeyProc[0] ) );
		pc->_KeyProc[0] = Key;
		pc->n_KeyProc++;
	}
}

//---------------------------------------------------------------------------

void SetControlText(PSI_CONTROL pc, CTEXTSTR text )
{
	SetCommonText( pc, text );
}

#undef ControlType
INDEX ControlType( PSI_CONTROL pc )
{
	if( pc )
		return pc->nType;
	return INVALID_INDEX;
}
//---------------------------------------------------------------------------

void SetCommonTransparent( PCONTROL pc, LOGICAL bTransparent )
{
	if( pc )
	{
		// if we are setting to transparent NO, then remove OriginalImage
		if( !(pc->flags.bTransparent = bTransparent ) )
		{
			lprintf( WIDE( "Turning off tansparency, so we don't need the background image now" ) );
			if( pc->OriginalSurface )
			{
				lprintf( WIDE("Early destruction of original surface image...") );
				UnmakeImageFile( pc->OriginalSurface );
				pc->OriginalSurface = NULL;
			}
		}
	}
}

//---------------------------------------------------------------------------

PSI_PROC( void, GetFramePosition )( PSI_CONTROL pf, int *x, int *y )
{
	if( pf )
	{
		if( x )
			(*x) = pf->original_rect.x;
	if( y )
		(*y) = pf->original_rect.y;
	}
}

//---------------------------------------------------------------------------

PSI_PROC( void, GetFrameSize )( PSI_CONTROL pf, int *w, int *h )
{
	if( pf )
	{
		if( w )
			(*w) = pf->original_rect.width;
		if( h )
			(*h) = pf->original_rect.height;
	}
}

CTEXTSTR GetControlTypeName( PSI_CONTROL pc )
{
	TEXTCHAR mydef[32];
	snprintf( mydef, sizeof( mydef ), PSI_ROOT_REGISTRY WIDE("/control/%d"), pc->nType );
	return GetRegisteredValueExx( mydef, NULL, WIDE("type"), FALSE );
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


PSI_NAMESPACE_END

//---------------------------------------------------------------------------
