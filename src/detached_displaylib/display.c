// --  see option falgs within displaystruc.h --
#include <stdarg.h>
#include <stdhdrs.h>
#include <sack_types.h>
#include <idle.h>
#include <logging.h>
#include <sharemem.h>
#include <timers.h>
#include <configscript.h>
#ifdef __LINUX__
// LoadPrivateFunctionEx for real_image library.
#include <system.h>
#include <fcntl.h>
#endif

#define GLOBAL_STRUCTURE_DEFINED
#include "global.h"

#include "spacetree.h"

struct saved_location
{
	S_32 x, y;
	_32 w, h;
};

struct sprite_method_tag
{
	PRENDERER renderer;
	Image original_surface;
   Image debug_image;
	PDATAQUEUE saved_spots;
	void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h );
   PTRSZVAL psv;
};


RENDER_NAMESPACE
//#define DEBUG_KEYBOARD_HANDLING
//#define MOUSE_CURSOR_DEBUG
//#define MOUSE_EVENT_DEBUG
//#define ADD_DELETE_USE_DEBUG
//#define __JIM_BUILD__
#ifdef __JIM_BUILD__
#define DEBUG_MOUSE_OWN
#define DEBUG_UPDATE_REGION
#define REDRAW_DEBUG
#define DEBUG_DIRTY_RECT
#define DEBUG_FOCUS_EVENTS
#endif
#ifdef __arm__
#define ALLOW_DIRECT_UPDATE

static void Blit_RGB888_RGB565( Image pifDest, Image pifSrc, int xofs, int yofs, int width, int height );
#endif
#ifdef __RAW_FRAMEBUFFER__
#define ALLOW_DIRECT_UPDATE
#endif
/*
struct saved_location
{
	S_32 x, y;
	_32 w, h;
};

struct sprite_method_tag
{
	PRENDERER renderer;
	Image original_surface;
   Image debug_image;
	PDATAQUEUE saved_spots;
	void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h );
   PTRSZVAL psv;
};
*/


#undef Allocate
#define Allocate(n)  HeapAllocate(pDisplayHeap, n )
RENDER_PROC( void, ClearDisplayEx )( PPANEL pPanel DBG_PASS );
#define ClearDisplay(d) ClearDisplayEx(d DBG_SRC)
RENDER_PROC( void, UpdateDisplayEx )( PPANEL pPanel DBG_PASS );
//#define UpdateDisplay(d) UpdateDisplayEx(d DBG_SRC )

//static PENDING_RECT update_rect;
static S_32 mouse_x, mouse_y;
static _32  mouse_buttons, _mouse_buttons;

enum thread_message{
    THREAD_DO_NOTHING
, THREAD_DO_SETMOUSEPOS
//, THREAD_DO_MOUSE_EVENT // mice events arrive in the thread anyhow?
, THREAD_DO_UPDATE_RECT
, THREAD_DO_EXIT
};

typedef struct pending_message_tag
{
	enum thread_message ID;
	union {
		struct {
			S_32 x, y;
		} mousepos;
      PENDING_RECT update_rect;
	}data;
} PENDING_MESSAGE;

// many mouse messages collapsed...
// many redraw events... also collapsed...
#define MAX_THREAD_MESSAGES 128
CRITICALSECTION csThreadMessage;
INDEX thread_head, thread_tail;
PENDING_MESSAGE thread_messages[MAX_THREAD_MESSAGES];


static void CPROC DefaultExit( PTRSZVAL psv, _32 keycode )
{
   exit(0);
}


//#if 0
//#ifndef DISPLAY_SERVICE
//#undef GetDisplayInterface
#undef DropDisplayInterface
extern POINTER CPROC DisplayGetDisplayInterface(void );
extern void CPROC DisplayDropDisplayInterface( POINTER );

PRIORITY_PRELOAD( RegisterDisplayInterface, VIDLIB_PRELOAD_PRIORITY )
{
	lprintf( WIDE("Registering display interface") );
	RegisterInterface( WIDE("render" ), DisplayGetDisplayInterface, DisplayDropDisplayInterface );
	BindEventToKey( NULL, KEY_F4, KEY_MOD_ALT, DefaultExit, 0 );
}
//#endif
//#endif

void AddUpdateRegionEx( PPENDING_RECT update_rect, S_32 x, S_32 y, _32 wd, _32 ht DBG_PASS )
#define AddUpdateRegion(x,y,w,h) AddUpdateRegionEx( &update_rect,x,y,w,h DBG_SRC )
{
#ifdef __LINUX__
	if( !update_rect->flags.bTmpRect )
		EnterCriticalSec( &update_rect->cs );
#endif
	if( wd && ht )
	{
		if( update_rect->flags.bHasContent )
		{
#ifdef DEBUG_UPDATE_REGION
			_xlprintf( DBG_AVAILABLE DBG_RELAY )( WIDE("Adding (%") _32fs WIDE(",%") _32fs WIDE(")-(%") _32f WIDE(",%") _32f WIDE(") to (%") _32fs WIDE(",%") _32fs WIDE(")-(%") _32f WIDE(",%") _32f WIDE(")")
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
			if( x + wd > update_rect->x + update_rect->width )
				update_rect->width = ( wd + x ) - update_rect->x;

			if( y < update_rect->y )
			{
				update_rect->height += update_rect->y - y;
				update_rect->y = y;
			}
			if( y + ht > update_rect->y + update_rect->height )
				update_rect->height = ( y + ht ) - update_rect->y;
#ifdef DEBUG_UPDATE_REGION
			lprintf( WIDE("result (%") _32f WIDE(",%") _32f WIDE(")-(%") _32f WIDE(",%") _32f WIDE(")")
                , update_rect->x, update_rect->y
                , update_rect->width, update_rect->height
					 );
#endif
		}
		else
		{
			//_lprintf( DBG_AVAILABLE, WIDE("Setting (%d,%d)-(%d,%d)") DBG_RELAY
			//		 , x, y
         //       , wd, ht
			//		 );
			update_rect->x = x;
			update_rect->y = y;
			update_rect->width = wd;
			update_rect->height = ht;
		}
		update_rect->flags.bHasContent = 1;
	}
#ifdef __LINUX__
	if( !update_rect->flags.bTmpRect )
		LeaveCriticalSec( &update_rect->cs );
#endif
}

void CPROC DisplayBlatColor( Image image, S_32 x, S_32 y, _32 w, _32 h, CDATA color );
//---------------------------------------------------------------------------

static PMEM pDisplayHeap;


//---------------------------------------------------------------------------

static void AddUse( PPANEL panel )
{
#ifdef ADD_DELETE_USE_DEBUG
	lprintf( WIDE("Okay adding a display use %p"), panel );
#endif
	if( panel )
      panel->used++;
}

static PPANEL DeleteUse( PPANEL panel )
{
#ifdef ADD_DELETE_USE_DEBUG
	lprintf( WIDE("Okay deleteing a display use %p"), panel );
#endif
	if( panel )
	{
		if( panel->flags.destroy && panel->used == 1 )
		{
			panel->used = 0;
         lprintf( WIDE("YES! We can FINALLY Close this display.") );
			CloseDisplay( panel );
         return NULL;
		}
      panel->used--;
	}
   return panel;
}

//---------------------------------------------------------------------------

#ifdef _DEBUG_REGIONS_WITH_DRAW_
static void MarkPoint( PSPACEPOINT p )
{
	do_line( g.pRootPanel->common.RealImage
			 , p[0] - 4, p[1] - 4
			 , p[0] + 4, p[1] + 4
			 , Color(255,255,255) );
	do_line( g.pRootPanel->common.RealImage
			 , p[0] - 4, p[1] + 4
			 , p[0] + 4, p[1] - 4
			 , Color(255,255,255) );
	UpdateDisplayPortion( g.pRootPanel, p[0] - 4, p[1] - 4, 8, 8 );
}
#endif
//---------------------------------------------------------------------------

static void WriteToDisplay( IMAGE_RECTANGLE *rect_dirty )
{
#ifdef REDRAW_DEBUG
	lprintf( WIDE("Drawing update_rect (%d,%d)-(%d,%d)") ,rect_dirty->x
															, rect_dirty->y
															, rect_dirty->width
			 , rect_dirty->height );
#endif
#ifdef _WIN32
	// then commit it to the real device ( which may or may not have been done.
	g.RenderInterface->_UpdateDisplayPortionEx( (PRENDERER)g.hVideo
															, rect_dirty->x, rect_dirty->y
															, rect_dirty->width, rect_dirty->height
															 DBG_SRC);
	//update_rect.flags.bHasContent = FALSE;
#elif defined( __LINUX__ )
// this should be how frame buffers are handled
// had to post the update rect with SDL implementation.
// SDL had to synchronize to X event pipe...
#ifdef ALLOW_DIRECT_UPDATE
	EnterCriticalSec( &g.csUpdating );
#  ifdef __ARM__
	Blit_RGB888_RGB565( g.PhysicalSurface
							, g.RealSurface
							, rect_dirty->x, rect_dirty->y
							, rect_dirty->width, rect_dirty->height );
#  else
	BlotImageSizedTo( g.PhysicalSurface
						 , g.RealSurface
						 , rect_dirty->x, rect_dirty->y
						 , rect_dirty->x, rect_dirty->y
						 , rect_dirty->width, rect_dirty->height
						 );
#  endif
   LeaveCriticalSec( &g.csUpdating );
#else
	{
		extern void PostUpdateRect(   SPACEPOINT min, SPACEPOINT max );
		SPACEPOINT min, max;
      //lprintf( "Posting update rect Write to display?" );
		min[0] = rect_dirty->x;
		min[1] = rect_dirty->y;
		max[0] = rect_dirty->width;
		max[1] = rect_dirty->height;
		PostUpdateRect( min, max );
	}
#endif
#else
#error NO_PHYSICAL_SURFACE_UDPATE
#endif
}

//---------------------------------------------------------------------------

//#ifdef _DEBUG_REGIONS_WITH_DRAW_
RENDER_NAMESPACE_END
DISPLAY_NAMESPACE
void DrawSpace( void *panel, PSPACEPOINT min, PSPACEPOINT max, CDATA color )
{
	// kinda need to keep this routine around - useful for clearing
   // the background areas that are not used...
//#ifdef REDRAW_DEBUG
#ifdef DEBUG_DIRTY_RECT
	lprintf( WIDE("Drawing region %p (%") _32fs WIDE(",%") _32fs WIDE(")-(%") _32fs WIDE(",%") _32fs WIDE(") %08") _32fx
			 , panel, min[0], min[1], max[0], max[1], color );
#endif
//#endif
#ifdef INTERNAL_BUFFER
#define IMG g.SoftSurface
#else
#define IMG g.RealSurface
#endif
//#define IMG g.pRootPanel->common.RealImage
	//if( min[0] == 0 && min[1] == 0 )
//   DebugBreak();
   FixImagePosition( IMG );
	g.ImageInterface->_BlatColorAlpha( IMG, min[0], min[1], max[0] - min[0]+1, max[1]-min[1]+1, color );

	if( color )
	{
	//SetAlpha( color, AlphaVal( color ) * 4 );
		color = SetAlpha( color, 255 );
	}
	do_vlineAlpha( IMG, min[0], min[1], max[1], color?color:Color(   0,  0,255 ) );
	do_vlineAlpha( IMG, max[0], min[1], max[1], color?color:Color( 255,  0,  0 ) );
	do_hlineAlpha( IMG, min[1], min[0], max[0], color?color:Color(   0,  0,255 ) );
	do_hlineAlpha( IMG, max[1], min[0], max[0], color?color:Color( 255,  0,  0 ) );
	do_lineAlpha(  IMG, min[0], min[1], max[0], max[1], color?color:Color( 0, 255,  0 ) );
	do_lineAlpha(  IMG, min[0], max[1], max[0], min[1], color?color:Color( 0, 255,  0 ) );

#ifdef INTERNAL_BUFFER
	g.ImageInterface->_BlotImageSizedEx( g.RealSurface, g.SoftSurface
												  , min[0], min[1]
												  , min[0], min[1]
												  , max[0] - min[0] + 1
												  , max[1] - min[1] + 1
												  , 0, BLOT_COPY );
#endif
#if defined( __LINUX__ )
#if !defined( __NO_SDL__ )
	SDL_UpdateRect( g.surface
					  , 0
					  , 0
					  , g.width
					  , g.height );
#elif defined( __RAW_FRAMEBUFFER )
	BlotImageSizedTo( g.PhysicalSurface
						 , g.RealSurface
						 , min[0], min[1]
						 , min[0], min[1]
						 , max[0] - min[0] + 1
						 , max[1] - min[1] + 1 )
#else
	IMAGE_RECTANGLE rect;
	rect.x = min[0];
	rect.y = min[1];
	rect.width = max[0] - min[0] + 1;
	rect.height = max[1] - min[0] + 1;
   WriteToDisplay( &rect );
#endif
#else // not linux, therefore windows?
	g.RenderInterface->_UpdateDisplayPortionEx( (PPANEL)g.hVideo, min[0], min[1]
														, max[0] - min[0] + 1
														, max[1] - min[1] + 1 DBG_SRC);
#endif
#ifdef DEBUG_DIRTY_RECT
	Log( WIDE("Done.") );
#endif
}
DISPLAY_NAMESPACE_END
RENDER_NAMESPACE
//#endif

//---------------------------------------------------------------------------

#ifdef _DEBUG_REGIONS_WITH_DRAW_
static void DrawSpaces2( void )
{
	PSPACENODE found = g.pSpaceRoot;        							 
	void *finddata = NULL;                                              
	SPACEPOINT min, max;
   lprintf( WIDE("Drawing all spaces...") );
	ClearImage( IMG );
	max[0] = ( min[0] = 0 ) + g.width - 1;       
	max[1] = ( min[1] = 0 ) + g.height - 1;      

	for( found = FindRectInSpace( found, min, max, &finddata );  
	     found;                                                  
	     found = FindRectInSpace( NULL, min, max, &finddata ) )  
	{                                                            
		PPANEL panel = GetNodeData( found );
		if( panel == g.pRootPanel )
			DrawSpace( panel, min, max, Color( 255,255,52) );
		else
			DrawSpace( panel, min, max, Color( 52,255,255) );

	}
	UpdateDisplay( g.pRootPanel );
}
#endif
//---------------------------------------------------------------------------

// Dispatch events is capable of collecting all region parts to be draw
// the bBehind flag adds all regions directly 'behind' the current to be drawn.
// this option is used in the case of CloseDIsplay which causes all panels
// behind the one dissappearing to be drawn.

					  // when updating to the renderer... the rect should be the maximal thing... probably NULL
					  // and panel should be the one which is being sync-rendered...
					  // this will only show the blocks that have changed for that display... and not those
					  // over it.

//---------------------------------------------------------------------------
#ifndef __ARM__
void PostUpdateRect(   SPACEPOINT min, SPACEPOINT max )
{
	EnterCriticalSec( &csThreadMessage );
#ifdef __LINUX__
//#ifdef ALLOW_DIRECT_UPDATE
//	Blit_RGB888_RGB565( g.RealSurface
//							, g.SoftSurface
//							, rect_dirty->x, rect_dirty->y
//							, rect_dirty->width, rect_dirty->height );
//
//#else // have to schedule SDL update...
	while( 1 )
	{
		INDEX new_tail, currmsg = thread_tail;
      //lprintf( WIDE("Post update...") );
		thread_messages[currmsg].data.update_rect.x = min[0];
		thread_messages[currmsg].data.update_rect.y = min[1];
		thread_messages[currmsg].data.update_rect.width = max[0];
		thread_messages[currmsg].data.update_rect.height = max[1];
      //lprintf( WIDE("Update queued for (%d,%d) (%d,%d)"), min[0], min[1], max[0], max[1] );
		thread_messages[currmsg].data.update_rect.flags.bHasContent = 1;
		thread_messages[currmsg].ID = THREAD_DO_UPDATE_RECT;
		//lprintf( WIDE("New update rectangle... %d,%d %d,%d"), min[0], min[1], max[0], max[1] );
		new_tail = thread_tail + 1;
		if( new_tail >= MAX_THREAD_MESSAGES )
			new_tail -= MAX_THREAD_MESSAGES;
		if( new_tail != thread_head )
		{
			thread_tail = new_tail;
			if( currmsg == thread_head )
			{
#if defined( __NO_SDL__ )
			// this is actually probably not even called, no SDL might
			// imply RAW_FRAMEBUFFER which outputs directly instead of waiting to
			// coordinate outputs.
            lprintf( "Send Serv... %ld", g.dwMsgBase );
				SendServiceEvent( 0, g.dwMsgBase + MSG_ThreadEventPost, NULL, 0 );
#else
				SDL_UserEvent event;
				event.type = SDL_USEREVENT;
				event.code = 0;
				event.data1 = NULL;
				event.data2 = NULL;
				SDL_PushEvent( (SDL_Event*)&event );
#endif
				// some sort of error might be expected here
				// in which case, on success, move the break following
            // as the condition of SUCCESS.
			}
			break;
		}
		xlprintf(LOG_ALWAYS)( WIDE("No Rooom to enque the mssage... retrying... %") _32fs WIDE(" %") _32fs WIDE(""), thread_head, thread_tail );
		LeaveCriticalSec( &csThreadMessage );
		EnterCriticalSec( &csThreadMessage );
	}
#endif
	LeaveCriticalSec( &csThreadMessage );
}
#endif

static void RenderUndirtyMouse( void )
{
	// find all nodes within the region covered by panel, if specified
	// otherwise find all regions within the rectangle...
	// collection requires a panel AND a rect, and merely adds
	// the rectangle described by panel into the rect, for later use
	// this allows collection of panels with the union
	// of rectangles specified by panels...
	// (most common, move panel, mark rect with original position
	//   then union that with the new position, dispatch to all within
   //   those bounds.)
// this is a debugging thing
	// optionally dump rect framed by LR()
#ifdef REDRAW_DEBUG
#  define LR(r)      {lprintf( #r " = %d,%d %d,%d", r.x, r.y, r.width, r.height );}
#else
#  define LR(r)
#endif

#ifdef INTERNAL_BUFFER
	PSPACENODE found;
	SPACEPOINT min, max;
	void *finddata = NULL;
#  ifdef DEBUG_DIRTY_RECT
	lprintf( WIDE("render undirty mouse (similar to dispatch redraw events)") );
#  endif

	if( !g.SoftCursor )
      return;
	max[0] = ( min[0] = g.SoftCursorX ) + g.SoftCursor->width - 1;
	max[1] = ( min[1] = g.SoftCursorY ) + g.SoftCursor->height - 1;

#  ifdef REDRAW_DEBUG
	lprintf( WIDE("finding panel(s) around (%") _32f WIDE(",%") _32f WIDE(")-(%") _32f WIDE(",%") _32f WIDE(")")
			 , min[0], min[1], max[0], max[1] );
   //DrawSpaces2();
#  endif
	{
		IMAGE_RECTANGLE rect_mouse, rect_clean, rect_mouse_clean, rect_newmouse;
		IMAGE_RECTANGLE last_cursor;
		last_cursor.x = g.last_cursor_x;
		last_cursor.y = g.last_cursor_y;
		last_cursor.width = g.SoftCursor->width;
		last_cursor.height = g.SoftCursor->height;
		rect_newmouse.x = g.SoftCursorX;
		rect_newmouse.y = g.SoftCursorY;
		rect_newmouse.width = g.SoftCursor->width;
		rect_newmouse.height = g.SoftCursor->height;
		MergeRectangle( &rect_mouse, &last_cursor, &rect_newmouse );
		//lprintf( WIDE("stats... mouse, last, cur") );
		LR( rect_mouse );
		LR( last_cursor );
      LR( rect_newmouse );
		max[0] = ( min[0] = rect_mouse.x ) + rect_mouse.width - 1;
		max[1] = ( min[1] = rect_mouse.y ) + rect_mouse.height - 1;
		//lprintf( WIDE("-------- Enter find rect %p"), finddata );
		EnterCriticalSec( &g.csSpaceRoot );

		for( found = FindRectInSpace( g.pSpaceRoot, min, max, &finddata );
			  found;
			  found = FindRectInSpace( NULL, min, max, &finddata ) )
		{
         IMAGE_RECTANGLE dirty_space;
         int bDirty = IsNodeDirty( found, &dirty_space );
			//if( !IsNodeDirty( found, &dirty_space ) )
			{
				rect_clean.x = min[0];
				rect_clean.y = min[1];
				rect_clean.width = max[0] - min[0] + 1;
				rect_clean.height = max[1] - min[1] + 1;
				if( IntersectRectangle( &rect_mouse_clean, &rect_mouse, &rect_clean ) )
				{
					LR( rect_clean );
               LR( rect_mouse_clean );
					if( g.flags.soft_cursor_on || g.flags.soft_cursor_was_on )
					{
						//BlotImageSizedEx( g.RealSurface, g.SoftSurface
						//					 , g.last_cursor_x, g.last_cursor_y
						//					 , g.last_cursor_x, g.last_cursor_y
						//					 , g.SoftCursor->width, g.SoftCursor->height
						//					 , 0, BLOT_COPY );
                  //lprintf( WIDE("...") );
						g.ImageInterface->_BlotImageSizedEx( g.RealSurface, g.SoftSurface
																	  , rect_mouse_clean.x, rect_mouse_clean.y
																	  , rect_mouse_clean.x, rect_mouse_clean.y
																	  , rect_mouse_clean.width, rect_mouse_clean.height
																	  , 0
																	  , BLOT_COPY );
						if( !bDirty && g.flags.soft_cursor_on )
						{
                     IMAGE_RECTANGLE rect_newmouse_clean;
							if( IntersectRectangle( &rect_newmouse_clean, &rect_newmouse, &rect_clean ) )
							{
								// cut back to just the new posisition within this region
								// (rect_newmouse setup a long time ago still has the original, NEW position)
								LR( rect_newmouse_clean );
                        LR( rect_newmouse );
								g.ImageInterface->_BlotImageSizedEx( g.RealSurface, g.SoftCursor
																			  , rect_newmouse_clean.x
																			  , rect_newmouse_clean.y
																			  , rect_newmouse_clean.x - rect_newmouse.x
																			  , rect_newmouse_clean.y - rect_newmouse.y
																			  , rect_newmouse_clean.width
																			  , rect_newmouse_clean.height
																			  , g.SoftCursorAlpha
																			  , BLOT_COPY
																			  );
							}
						}
						WriteToDisplay( &rect_mouse_clean );
					}
				}
			}
		}
		LeaveCriticalSec( &g.csSpaceRoot );
		//lprintf( WIDE("-------- Leave find rect %p"), finddata );
	}
#endif
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// pending rect (update_rect) will be ... an advisory rectangle indicating that
// at dirty nodes, only those and those portions of those dirty which are
					  // within this rectangle are actually updated to the display.
					  // if update_rect is NULL, it is assumed that any portion of this panel
// which is marked dirty will be updated to the display.
static void RenderDirtyRegionsEx( PPANEL panel DBG_PASS )
#define RenderDirtyRegions() RenderDirtyRegionsEx( NULL DBG_SRC)
{
	PSPACENODE node;
   IMAGE_RECTANGLE rect_dirty;
#ifdef INTERNAL_BUFFER
	IMAGE_RECTANGLE rect_mouse;
#endif
   //IMAGE_RECTANGLE rect_mouse_dirty;
	IMAGE_RECTANGLE rect_newmouse;
#if defined( DEBUG_REDRAW ) || defined( DEBUG_DIRTY_RECT )
	_xlprintf(1 DBG_RELAY)( WIDE("-------------- Rendering dirty regions.... ") );
#endif
	if( g.flags.in_mouse )
	{
		g.flags.was_in_mouse = 1;

#if defined( DEBUG_REDRAW )
		lprintf( WIDE("!!!! RETURN CAUSE MOUSE IS BUSY!") );
#endif
		return;
	}
// rect_dirty passed when display_image marks a dirty node
   // is expressed in x, y, width, and height, not an absolute
#ifdef INTERNAL_BUFFER
	if( g.flags.soft_cursor_on )
	{
		IMAGE_RECTANGLE last_cursor;
		last_cursor.x = g.last_cursor_x;
		last_cursor.y = g.last_cursor_y;
		last_cursor.width = g.SoftCursor->width;
		last_cursor.height = g.SoftCursor->height;
		rect_newmouse.x = g.SoftCursorX;
		rect_newmouse.y = g.SoftCursorY;
		rect_newmouse.width = g.SoftCursor->width;
		rect_newmouse.height = g.SoftCursor->height;
		MergeRectangle( &rect_mouse, &last_cursor, &rect_newmouse );
      LR(last_cursor);
		LR(rect_newmouse);
		LR(rect_mouse);

		//rect_mouse.x = g.SoftCursorX;
		//rect_mouse.y = g.SoftCursorY;
		//rect_mouse.width = g.SoftCursor->width;
		//rect_mouse.height = g.SoftCursor->height;
	}
#endif
	while( ( node = GetDirtyNode( NULL, &rect_dirty ) ) )
	{
      int bDraw;
		IMAGE_RECTANGLE rect_panel;
		IMAGE_RECTANGLE rect_merge;
		PSPACEPOINT realmin, realmax;
		PPANEL node_panel = (PPANEL)GetNodeData( node );
		realmin = GetSpaceMin( node );
		realmax = GetSpaceMax( node );
		rect_panel.x = realmin[0];
		rect_panel.y = realmin[1];
		rect_panel.width = (realmax[0] - realmin[0]) + 1;
		rect_panel.height = (realmax[1] - realmin[1]) + 1;
#ifdef DEBUG_DIRTY_RECT
		{
			PSPACEPOINT realmin, realmax;
			realmin = GetSpaceMin( node );
			realmax = GetSpaceMax( node );
			lprintf( WIDE("Have a dirty node for this panel %p... native (%") _32fs WIDE(",%") _32fs WIDE(")-(%") _32fs WIDE(",%") _32fs WIDE(") dirty (%") _32fs WIDE(",%") _32fs WIDE(") (%") _32fs WIDE(",%") _32fs WIDE("):(%") _32fs WIDE(",%") _32fs WIDE(")")
					 , GetNodeData( node )
					 , realmin[0], realmin[1], realmax[0], realmax[1]
					 , rect_dirty.x, rect_dirty.y
					 , rect_dirty.width, rect_dirty.height
					 , rect_dirty.width + rect_dirty.x
					 , rect_dirty.height + rect_dirty.y
					 );
			{
				SPACEPOINT min, max;
				min[0] = rect_dirty.x;
				min[1] = rect_dirty.y;
				max[0] = rect_dirty.width + rect_dirty.x;
				max[1] = rect_dirty.height + rect_dirty.y;
				DrawSpace( NULL, min, max, SetAlpha( BASE_COLOR_GREEN, 32 ) );
			}
		}
#endif
			LR(rect_dirty );

			if( g.flags.soft_cursor_on )
			{
            IMAGE_RECTANGLE rect_tmp;
				if( IntersectRectangle( &rect_merge, &rect_newmouse, &rect_panel ) )
				{
               rect_tmp = rect_dirty;
					MergeRectangle( &rect_dirty, &rect_tmp, &rect_merge );
               bDraw = TRUE;
				}
				else
               bDraw = FALSE;
            //rect_dirty = rect_merge;
			}
			//else
#ifdef INTERNAL_BUFFER
			{
				if( panel )
				{
#if defined( DEBUG_REDRAW )
					lprintf( "Copying stable image to soft image... %d, %d %d,%d", rect_dirty.x, rect_dirty.y, rect_dirty.width + 1, rect_dirty.height + 1 );
#endif
					g.ImageInterface->_BlotImageSizedEx( g.SoftSurface, panel->common.StableImage
																  , rect_dirty.x, rect_dirty.y
																  , rect_dirty.x-panel->common.x, rect_dirty.y-panel->common.y
																  , rect_dirty.width + 1// - min[0] + 1
																  , rect_dirty.height + 1// - min[1] + 1
																  , 0
																  , BLOT_COPY );
               /*
					g.ImageInterface->_BlotImageSizedEx( g.RealSurface, panel->common.StableImage
																  , rect_dirty.x, rect_dirty.y
																  , rect_dirty.x-panel->common.x, rect_dirty.y-panel->common.y
																  , rect_dirty.width + 1// - min[0] + 1
																  , rect_dirty.height + 1// - min[1] + 1
																  , 0
																  , BLOT_COPY );
                                                  */
				}
				if( node_panel )
				{
#if defined( DEBUG_REDRAW )
			               lprintf( "Copying dirty node stable image to soft image... %d, %d %d,%d", rect_dirty.x, rect_dirty.y, rect_dirty.width + 1, rect_dirty.height + 1 );
#endif
					g.ImageInterface->_BlotImageSizedEx( g.SoftSurface, node_panel->common.StableImage
																  , rect_dirty.x, rect_dirty.y
																  , rect_dirty.x-node_panel->common.x, rect_dirty.y-node_panel->common.y
																  , rect_dirty.width + 1// - min[0] + 1
																  , rect_dirty.height + 1// - min[1] + 1
																  , 0
																  , BLOT_COPY );
				}
				//else
#if defined( DEBUG_REDRAW )
			               lprintf( "Copying soft image to real image... %d, %d", rect_dirty.x, rect_dirty.y );
#endif
					g.ImageInterface->_BlotImageSizedEx( g.RealSurface, g.SoftSurface
																  , rect_dirty.x, rect_dirty.y
																  , rect_dirty.x, rect_dirty.y
																  , rect_dirty.width + 1// - min[0] + 1
																  , rect_dirty.height + 1// - min[1] + 1
																  , 0
																  , BLOT_COPY );
				if( bDraw && g.flags.soft_cursor_on )
				{
					LR(rect_panel);
					LR(rect_newmouse);
					g.ImageInterface->_BlotImageSizedEx( g.RealSurface, g.SoftCursor
																  , rect_merge.x
																  , rect_merge.y
																  , rect_merge.x - rect_newmouse.x
																  , rect_merge.y - rect_newmouse.y
																  , rect_merge.width
																  , rect_merge.height
																  , g.SoftCursorAlpha
																  , BLOT_COPY
																  );
				}
			}
#endif
			WriteToDisplay( &rect_dirty );
	}
/*
               */
   RenderUndirtyMouse();
}

//---------------------------------------------------------------------------

static void InvokeRedrawMethod( PPANEL draw, PPENDING_RECT rect )
{
	if( draw->RedrawMethod )
	{
		EnterCriticalSec( &g.csUpdating );
		if( rect && rect->flags.bHasContent )
		{
			draw->common.dirty.x = rect->x;
			draw->common.dirty.y = rect->y;
			draw->common.dirty.width = rect->width;
			draw->common.dirty.height = rect->height;
#ifdef DEBUG_DIRTY_RECT
			lprintf( WIDE("!!!!! Set a dirty rectangle to draw into (further clip image ops)") );
#endif
			draw->common.flags.dirty_rect_valid = 1;
		}
		else
		{
			// lprintf( WIDE("DO NOT Set a dirty rectangle whole surface is dirty.") );
			draw->common.flags.dirty_rect_valid = 0;
		}
		//lprintf( WIDE("Draw method of %p"), draw );
		draw->RedrawMethod( draw->psvRedraw
								, (PRENDERER)draw );

		LeaveCriticalSec( &g.csUpdating );
#ifndef DISPLAY_SERVICE
		{
			// if this is a service compilation, then the remote
			// process will result with a syncrender type responce
			// after the update has been dispatched to the child...
			// this will then appear in queue after all child draw
			// events... ensuring a complete update of the client
			// surface by event before the draw...
			// therefore these regions do not get drawn....
#ifdef DEBUG_DIRTY_RECT
			//lprintf( WIDE("Drawing dirty nodes of panel %p"), panel );
#endif
						// why only this one for a DISPLAY_SERVICE mode?
						// oh - cause we don't get to know when the draw happens
						// the draw method is a generated event to the application(s)
						// ... but...
						// okay but then this should only touch the regions that are visible
						// to the very top - since this application is really in charge
						// of the base display surface, it should be safe to do this.
			// (that event will have completed locally anyway)
			if( draw == g.pRootPanel )
			{
				// actually this results in noisy updates...
				// partial application draws, etc.
				// the redraw method when used as a service should
				// result in sync_render at the completion of dispatched
				// events to the application layer.
				// we will have already done our dirty work to the surface
				// and anything dirtied will be draw... there are not filters
				// available anymore for dirty rectangles and panel specific dirty check.
				// -- SO DON'T DO THIS!?---
				// RenderDirtyRegionsEx( DBG_VOIDSRC );
			}
		}
#endif
	}
	else
		lprintf( WIDE("Panel had no redraw method...") );
}

static void DispatchDrawEventsEx( PPENDING_RECT rect, PPANEL panel, int bCollect DBG_PASS )
#undef DispatchRedrawEvents
#define DispatchRedrawEvents(p,c) DispatchDrawEventsEx(NULL,p,c DBG_SRC )
#define DispatchDrawEvents(r,p,c) DispatchDrawEventsEx(r,p,c DBG_SRC )
{
	// find all nodes within the region covered by panel, if specified
	// otherwise find all regions within the rectangle...
	// collection requires a panel AND a rect, and merely adds
	// the rectangle described by panel into the rect, for later use
	// this allows collection of panels with the union
	// of rectangles specified by panels...
	// (most common, move panel, mark rect with original position
	//   then union that with the new position, dispatch to all within
   //   those bounds.)
	PSPACENODE found;
	SPACEPOINT min, max;
	void *finddata = NULL;
#ifdef DEBUG_DIRTY_RECT
	_xlprintf( DBG_AVAILABLE DBG_RELAY)( WIDE("Dispatch redraw events %s"), bCollect?"collect":"dispatch" );
#endif
	if( rect && panel )
	{
		AddUpdateRegionEx( rect
							  , panel->common.x, panel->common.y
							  , panel->common.width, panel->common.height
								DBG_RELAY );
#ifdef DEBUG_UPDATE_REGION
		_xlprintf( 1 DBG_RELAY )( WIDE("Added panel region (%d,%d)-(%d,%d) result (%") _32fs WIDE(",%") _32fs WIDE(")-(%") _32f WIDE(",%") _32f WIDE(")")
				 , panel->common.x
				 , panel->common.y
				 , panel->common.width
				 , panel->common.height
				 , rect->x
				 , rect->y
				 , rect->width
				 , rect->height );
#endif
	}
	if( bCollect )
	{
#ifdef DEBUG_UPDATE_REGION
      lprintf( WIDE("And that panel region is all we're doing ... collecting some dirty") );
#endif
      return;
	}

	if( rect )
	{
		max[0] = ( min[0] = rect->x ) + rect->width - 1;
		max[1] = ( min[1] = rect->y ) + rect->height - 1;
	}
	else if( panel )
	{
		max[0] = ( min[0] = panel->common.x ) + panel->common.width - 1;
		max[1] = ( min[1] = panel->common.y ) + panel->common.height - 1;
	}

#ifdef REDRAW_DEBUG
	lprintf( WIDE("finding panel(s) around (%") _32f WIDE(",%") _32f WIDE(")-(%") _32f WIDE(",%") _32f WIDE(")"), min[0], min[1], max[0], max[1] );
   //DrawSpaces2();
#endif
	{
		int single_panel = 1;
		PPANEL draw = NULL;
		PPANEL _draw = NULL;
						 //lprintf( WIDE("-------- Enter find rect %p"), finddata );
		EnterCriticalSec( &g.csSpaceRoot );

		for( found = FindRectInSpace( g.pSpaceRoot, min, max, &finddata );
			  found;
			  found = FindRectInSpace( NULL, min, max, &finddata ) )
		{
			//lprintf( WIDE("Adding a rect...") );
			draw = (PPANEL)GetNodeData( found );
			//lprintf( WIDE("-------- In find rect %p"), finddata );
			if( draw )
			{
#ifdef REDRAW_DEBUG
				lprintf( WIDE("Found panel %p marking it dirty"), draw );
#endif
			// when we rebuild the tree... should
            // result with space-parts of panels that are dirty that ARE dirty.
				draw->flags.dirty = TRUE;
				if( _draw && draw != _draw )
				{
#ifdef REDRAW_DEBUG
					lprintf( WIDE("more than one drawable panel was found.") );
#endif
					single_panel = 0;
				}
				_draw = draw;
			}
			else
				lprintf( WIDE("Found node with no panel") );
		}
		LeaveCriticalSec( &g.csSpaceRoot );
		//lprintf( WIDE("-------- Leave find rect %p"), finddata );
		if( single_panel )
		{
#ifdef REDRAW_DEBUG
			lprintf( WIDE("Single panel update") );
#endif
			if( draw && draw->flags.dirty )
			{
            InvokeRedrawMethod( draw, rect );
				RenderDirtyRegionsEx( draw DBG_RELAY );
			}
			draw->flags.dirty = FALSE;
#ifdef REDRAW_DEBUG
			{
				if( !draw ) lprintf( WIDE("Panel to draw is NULL") );
				else if( !draw->flags.dirty ) lprintf( WIDE("panel is not dirty.") );
			}
#endif
		}
		else
		{
#ifdef REDRAW_DEBUG
			lprintf( WIDE("Any dirty panel update") );
#endif
			panel = g.pRootPanel;
			while( panel )
			{
				if( panel->flags.dirty )
				{
					InvokeRedrawMethod( panel, rect );
					RenderDirtyRegions( );
					panel->flags.dirty = FALSE;
				}
				panel = panel->below; // go the the one I am below.
			}
#ifndef DISPLAY_SERVICE
         // if it's not in a mouse dispatch perhaps a timer update hit us?
			if( !g.flags.in_mouse )
			{
            lprintf( WIDE("All drawn, render these now.") );
				RenderDirtyRegions();
			}
			else
			{
				// a update happened, but so we don't
				// trash the mouse image, we delay
				// and allow the mouse to render itself and
            // these dirty regions.
				g.flags.was_in_mouse = 1;
			}
#endif
		}
	}
}

//---------------------------------------------------------------------------


void Redraw(PRENDERER hVideo)
{
	InvokeRedrawMethod( hVideo, NULL );
	RenderDirtyRegionsEx( hVideo DBG_SRC );

	return;
}


//---------------------------------------------------------------------------

#ifdef __LINUX__
#ifndef __arm__
static void DrawAllPanels( void )
{
   // no rect needed...
	DispatchRedrawEvents( g.pRootPanel, FALSE );
}
#endif
#endif

//---------------------------------------------------------------------------

RENDER_PROC( void, OwnMouseEx )( PPANEL panel, _32 own DBG_PASS )
{
	// all most messages get sent thos this panel's method.

	// if set own, and there was no previous owner
	//        then we can set an owner
	// else if clear own, and the panel specified is the one
	// which is the current owner, can disable it.
	if( own )
	{
#ifdef DEBUG_MOUSE_OWN
		Log2( "To own: %p  previous: %p", panel, g.pMouseOwner );
#endif
		if( ( ( !g.pMouseOwner && panel ) // able to own.
			  ||( g.flags.auto_owned_mouse ) // internal ownership only...
			 ) )
		{
#ifdef DEBUG_MOUSE_OWN
			_lprintf( DBG_RELAY )( "Mouse is NOW owned %p.", panel );
#endif
			g.pMouseOwner = panel;
			g.flags.auto_owned_mouse = 0;
			//return TRUE;
			return;
		}
		else if( g.pMouseOwner )
		{
#ifdef DEBUG_MOUSE_OWN
			lprintf( "check to see if the previous owner is this one's parent.. %p %p %p %p"
					 , panel
					 , panel->parent
					 , g.pMouseOwner
					 , g.pMouseOwner->parent );
#endif
			if( g.pMouseOwner->common.flags.invisible || panel->parent == g.pMouseOwner )
			{
#ifdef DEBUG_MOUSE_OWN
				Log( "Okay to own the mouse...a parent was the previous owner (or maybe owner is invisible?)" );
#endif
				g.pMouseOwner = panel;
				g.flags.auto_owned_mouse = 0;
				panel->flags.owning_parents_mouse = 1;
				//return TRUE;
				return;
			}
		}
	}
	else
	{
		if( g.pMouseOwner == panel )
		{
			// and - only return truth on successful set or clear
			// of the owner.
			// maybe we should own it per thread?
			// but there's only one thread?
			if( panel->MouseMethod )
			{
				AddUse( panel );
				//lprintf( WIDE("Adding Use... %p"), panel );
				panel->MouseMethod( panel->psvMouse
										, mouse_x - panel->common.x
										, mouse_y - panel->common.y
										, mouse_buttons );
				//lprintf( WIDE("going to delete that use.") );
				panel = DeleteUse( panel );
			}
			if( panel && g.pMouseOwner == panel )
			{
				if( panel->flags.owning_parents_mouse )
				{
					g.pMouseOwner = panel->parent;
					panel->flags.owning_parents_mouse = 0;
				}
				else
				{
				//Log1( DBG_FILELINEFMT "Mouse is no longer owned." DBG_RELAY, 0 );
					g.pMouseOwner = NULL;
					g.flags.auto_owned_mouse = 0;
				}
			}
			//return TRUE;
			return;
		}
	}
	//Log4( WIDE("Panel: %p parnet: %p current owner: %p own: %d")
	//	 , panel
	//	 , panel->parent
   //    , g.pMouseOwner
	//	 , own );
   //Log1( DBG_FILELINEFMT "Mouse ownership did not change." DBG_RELAY, 0 );
	//return FALSE;
   return;
}

//---------------------------------------------------------------------------
void SetFocusedPanel( PPANEL panel )
{
	if( !panel ) // don't ever lose all focus...
	{
#ifdef DEBUG_FOCUS_EVENTS
		Log( WIDE("Setting NULL focused panel...") );
#endif
	}
#ifdef DEBUG_FOCUS_EVENTS
	else
		lprintf( WIDE("Setting focused panel %p"), panel );
#endif
	if( panel && panel->flags.destroy )
	{
      lprintf( WIDE("Don't focus panel being destroyed.") );
      return;
	}
   // check children - perhaps we have to force the focus to some child
	// unless it's that child that's setting the focus out...
	// but for now - let's just direct focus anywhere.

	if( !g.pFocusedPanel &&
		panel &&
		g.pLastFocusedPanel &&
		( g.pLastFocusedPanel != panel ) )
	{
#ifdef DEBUG_FOCUS_EVENTS
		lprintf( WIDE("No current focus, mark last focus for when we get it back...") );
#endif
		g.pLastFocusedPanel = panel;
	}
	else
	{
#ifdef DEBUG_FOCUS_EVENTS
		lprintf( "Focusing new panel..." );
#endif
		if( g.pFocusedPanel != panel )
		{
			if( g.pFocusedPanel )
			{
			// Generate lose focus...
				if( g.pFocusedPanel->FocusMethod )
				{
#ifdef DEBUG_FOCUS_EVENTS
					lprintf( WIDE("Sending a lose focus message to panel...") );
#endif
					g.pFocusedPanel->FocusMethod( g.pFocusedPanel->psvFocus
														 , panel?panel:(PPANEL)1 );
				}
// g.pLastFocusedPanel
				if( g.pLastFocusedPanel )
				{
			// clear keyboard states on focus loss (for now...)
				MemSet( g.pLastFocusedPanel->KeyboardState
						, 0
						, sizeof( g.pLastFocusedPanel->KeyboardState ) );
				MemSet( g.pLastFocusedPanel->KeyboardMetaState
						, 0
						, sizeof( g.pLastFocusedPanel->KeyboardMetaState ) );
				MemSet( g.pLastFocusedPanel->KeyDownTime
						, 0
						, sizeof( g.pLastFocusedPanel->KeyDownTime ) );
				}

			}
			g.pFocusedPanel = panel;
								 // Generate gain focus...
			if( panel && panel->FocusMethod )
			{
#ifdef DEBUG_FOCUS_EVENTS
				lprintf( WIDE("Sending a gain focus message to panel...") );
#endif
				panel->FocusMethod( panel->psvFocus
										, NULL );
			}
		}
	}
}

//--------------------------------------------------------------------------

void CPROC LoseFocusHandler( PTRSZVAL psv, PRENDERER bLose )
{
	static int handling;
	if( handling )
		return;	
	handling = 1;
	if( bLose )
	{
#ifdef DEBUG_FOCUS_EVENTS
		lprintf( WIDE("Display Lose focuse %p has it now"), g.pFocusedPanel );
#endif
      if( g.pFocusedPanel )
			g.pLastFocusedPanel = g.pFocusedPanel;
		SetFocusedPanel( NULL ); // no focused panel.
	}
	else
	{
#ifdef DEBUG_FOCUS_EVENTS
		lprintf( WIDE("Display Gain focus, set panel to last focused?! %p (current %p)"), g.pLastFocusedPanel, g.pFocusedPanel );
#endif
		//g.pFocusedPanel = g.pLastFocusedPanel;
		if( g.pLastFocusedPanel )
			SetFocusedPanel( g.pLastFocusedPanel );
	}

	handling = 0;
}

//---------------------------------------------------------------------------
int PointIsInPanel( void *data, PSPACEPOINT p )
{
	PPANEL pPanel = (PPANEL)data;
#ifdef MOUSE_EVENT_DEBUG
	lprintf( WIDE("Panel has mouse in it...") ); // top down seach, right?
#endif
	if( g.flags.auto_owned_mouse && !mouse_buttons )
	{
		if( g.pMouseOwner )
		{
			// dispatch release event, and then disown.
			OwnMouse( g.pMouseOwner, FALSE );
			g.flags.auto_owned_mouse = 0;
			_mouse_buttons = mouse_buttons;
			// this event dispatched.
			return 1;
		}
	}

   // first is the top - any other is below that area automagically.
	if( pPanel->common.flags.backdrop_custom &&
	    pPanel->common.background.custom.PointOnPanel )
	{
		// background gets the mouse first, then normal
      // mouse messages are generated to the surface.
		pPanel->common.background.custom.PointOnPanel( pPanel->common.background.custom.psvUserOn
																	, pPanel, p );
   }
	if( pPanel->MouseMethod )
	{
      // if button has gone done.
		if( ( mouse_buttons ^ _mouse_buttons ) & mouse_buttons )
		{
         //lprintf( WIDE("New buttons... set focus and junk?") );
         // button press is on some other panel...
			if( pPanel != g.pFocusedPanel )
			{
#ifdef DEBUG_FOCUS_EVENTS
				lprintf( WIDE("Wow new focus...") );
#endif
				SetFocusedPanel( pPanel );
				if( pPanel != g.pTopMost
					// && (not mouse_over_focus...)
				  )
				{
               //lprintf( WIDE("Promoting panel") );
					PutDisplayAbove( pPanel, g.pTopMost );
				}
			}
		}

		if( mouse_buttons )
		{
			//Log( WIDE("Locking the mouse to this panel...") );
			if( !g.pMouseOwner )
			{
				OwnMouse( pPanel, TRUE );
				g.flags.auto_owned_mouse = 1;
			}
		}

		// else disown the mouse.. nah, cause then I can click, drag beyond border
		// and the owned mouse stays because of the mouse_button down.
		if( pPanel->psvMouse == 0xFACEBEAD )
			Log( WIDE("About to die...") );
#ifdef MOUSE_EVENT_DEBUG
		lprintf( WIDE("Dispatch mouse to application.") );
#endif
		AddUse( pPanel );
      //lprintf( "dispatch mouse %d %d %08x", p[0], p[1], mouse_buttons );
		if( pPanel->MouseMethod( pPanel->psvMouse, p[0]
									  , p[1]
									  , mouse_buttons ) )
		{
			pPanel = DeleteUse( pPanel );
			_mouse_buttons = mouse_buttons;
			return 1; // if the mouse method returns (TRUE) don't pass on.
		}
		else
		{
		}
		pPanel = DeleteUse( pPanel );
		// otherwise the mouse was ignored, and
      // it should be passed on to next layer.
		_mouse_buttons = mouse_buttons;
	}
#ifdef MOUSE_EVENT_DEBUG
	else
	{
      lprintf( WIDE("Panel has no mouse method...") );
	}
#endif
	// if panel has no mouse, then it must
	// be excluded from mouse events, and assumed to be
	// transparent to the mouse... There are only
	// very rare cicumstances that a panel does not
	// have a mouse method.
   // hmm nah, don't be transparent...
   return 1;
}

//---------------------------------------------------------------------------


// sequence of events
//   extern application draws data, and that data has nothing
// to do with the mouse, old, or new, it doesn't matter....
//   has to do with the previous cursor position, but it has
// bee updated...
//   the mouse moves.

// external even schedules draw under linux
// under windows, the draw is done directly.
// If scheduled, then the scheduled handler needs to draw
// the mouse cursor if that overlaps the current position...
// if unscheduled, then the mouse handler can update directly.


int RenderMouseEx( //PPENDING_RECT rect,
						int bUpdate
						DBG_PASS )
#define RenderMouse(u) RenderMouseEx( u DBG_SRC )
{
	// figures out if the mouse is on or off based on input buttons...
#ifdef INTERNAL_BUFFER
   _32 tmp_buttons = mouse_buttons& (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON);
	_32 delta_buttons = (tmp_buttons) ^ g._last_buttons;
	_32 last_buttons = g._last_buttons;
#endif
	//SPACEPOINT min, max;
   // might as well set this here
   //g._last_buttons = tmp_buttons;
	// should check to see if the current mouse image is in this rectangle
	// and if so - apply that to the display... (blot the orginal surface, then overblot the mouse)
	// these are mostly silly - I mean on down? what good is that?
	// well if we had an animaged cursor sequence (explosion) that would be good to start ondown/onup.
#ifdef INTERNAL_BUFFER
	if( g.SoftCursor )
	{
 		// still update the state... for some reason
		// we'll abort, but we want to maintain
		// what goes on stays on
		if( ( bUpdate && delta_buttons ) || g.flags.soft_cursor_always)
		{
#ifdef MOUSE_CURSOR_DEBUG
			_xlprintf( 1 DBG_RELAY )( WIDE("render mouse... delta %08lx new %08lx"), delta_buttons, tmp_buttons );
			lprintf( WIDE("Cursor was %s"), g.flags.soft_cursor_on?"ON":"off" );
			lprintf( WIDE("Processing new button state, which affects cursor..") );
#endif
			g.flags.soft_cursor_on = 0;
			if( g.flags.soft_cursor_always )
				g.flags.soft_cursor_on = 1;
			else if( g.flags.soft_cursor_down && tmp_buttons )
			{
				g.flags.soft_cursor_on = 1;
			}
			else if( g.flags.soft_cursor_up && !tmp_buttons )
				g.flags.soft_cursor_on = 1;
			else if( g.flags.soft_cursor_on_down )
			{
				if( delta_buttons && ( delta_buttons & tmp_buttons ) )
					g.flags.soft_cursor_on = 1;
			}
			else if( g.flags.soft_cursor_on_up )
			{
				if( delta_buttons && ( delta_buttons & last_buttons ) )
					g.flags.soft_cursor_on = 1;
			}
#ifdef MOUSE_CURSOR_DEBUG
			lprintf( WIDE("Cursor is now %s"), g.flags.soft_cursor_on?"ON":"off" );
#endif
		}
		// else - leave soft_cursor_on on - there is no change...
      // and was_on is correctly updated...

		if( g.flags.soft_cursor_on ||
			g.flags.soft_cursor_was_on )
		{
			RenderUndirtyMouse();
         // dirty mouse update will be okay
		}
	}
#endif
   return FALSE;
}


//---------------------------------------------------------------------------

static int CPROC Mouse( PTRSZVAL unused, S_32 x, S_32 y, _32 b )
{
	SPACEPOINT p;
	mouse_x = x;
   mouse_y = y;
#if defined( _WIN32 ) || defined( __NO_SDL__ )
   // sdl method saves this already
	g._last_buttons = mouse_buttons;
   mouse_buttons = b;
#endif
   g.flags.in_mouse = 1;
#ifdef MOUSE_EVENT_DEBUG
	lprintf( WIDE("Mouse: %d,%d %08x"), x, y, b );
#endif
   if( g.pMouseOwner && b )
   {
		// as long as a button is still down,
		// the mouse owner gets all buttons.
		// when the button goes up, it gets that
		// last notification, and then loses
		// ownership, if it has been auto owned...
		// if ownership was granted by application code
	// the owner will remain until disowning itself.
#ifdef MOUSE_EVENT_DEBUG
		lprintf( WIDE("someone owns mouse...") );
#endif
		p[0] = x - g.pMouseOwner->common.x;
		p[1] = y - g.pMouseOwner->common.y;
		if( g.pMouseOwner->psvMouse == 0xFACEBEAD )
			Log( WIDE("About to die...") );
		if( g.pMouseOwner->MouseMethod )
		{
         PPANEL panel = g.pMouseOwner;
#ifdef MOUSE_EVENT_DEBUG
			lprintf( WIDE("message to owner.") );
#endif
         AddUse( panel );
			panel->MouseMethod( panel->psvMouse
									, p[0], p[1]
									, mouse_buttons );
			panel = DeleteUse( panel );
		}
	}
   else
   {
	   p[0] = mouse_x = x;
		p[1] = mouse_y = y;
#ifdef MOUSE_EVENT_DEBUG
		lprintf( WIDE("finding owner for mouse...") );
#endif
		EnterCriticalSec( &g.csSpaceRoot );
		FindPointInSpace( g.pSpaceRoot, p, PointIsInPanel );
		LeaveCriticalSec( &g.csSpaceRoot );

	}
#ifdef INTERNAL_BUFFER
	if( g.SoftCursor )
	{
		g.flags.soft_cursor_was_on = g.flags.soft_cursor_on;
		g.last_cursor_x = g.SoftCursorX;
		g.last_cursor_y = g.SoftCursorY;
		g.SoftCursorX = mouse_x - g.SoftCursorHotX;
		g.SoftCursorY = mouse_y - g.SoftCursorHotY;
		// render with update... but then we go ahead and render it anyhow?
		// render might also have to be delayed until dirty regions under me
		// are done...
		RenderMouse( TRUE );
		//RenderUndirtyMouse();
      //lprintf( WIDE("And then we were done with the mouse...") );
	}
	else
		if( g.debug_log )
      		lprintf( WIDE("No cursor image...") );
#endif
	g.flags.in_mouse = 0;
	// anything which is in a dirty rect will be udpated
	// if the application is far lagged, everything will be undirty
	// it will begin drawing and eventually SyncRender, which will cause
	// any dirty regions under the mouse to be rendered again.

   if( g.flags.was_in_mouse )
	{
		// region updates are blocked by the 'in_mouse' flag,
		// this does the work that would have been done if
      // we weren't actively updating the mouse...
      g.flags.was_in_mouse = 0;
		RenderDirtyRegions( );
	}

	return 0;
}

//---------------------------------------------------------------------------


typedef struct key_equate_tag
{
	_8 common_code;
   _8 numalts;
   _8 base_codes[2]; // so far we only have sets of dual keys
} KEY_EQUATE, *PKEY_EQUATE;
KEY_EQUATE KeyEquates[256];



void InitKeyEquates( void )
{
   // if this is non zero we've already populated these
	if( KeyEquates[KEY_LEFT_SHIFT].common_code )
		return;
#ifndef WIN32
#define SETEQUATE(n,code,numcode,alt1,alt2) if( n < 256) { KeyEquates[n].common_code=code;KeyEquates[n].numalts=numcode;KeyEquates[n].base_codes[0]=alt1;KeyEquates[n].base_codes[1]=alt2;}


	SETEQUATE(KEY_LEFT_SHIFT, KEY_SHIFT, 2,  KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT  )        ;
	SETEQUATE(KEY_RIGHT_SHIFT, KEY_SHIFT, 2,  KEY_LEFT_SHIFT, KEY_RIGHT_SHIFT  )        ;
	SETEQUATE(KEY_LEFT_CONTROL, KEY_CONTROL, 2,  KEY_LEFT_CONTROL, KEY_RIGHT_CONTROL  ) ;
	SETEQUATE(KEY_RIGHT_CONTROL, KEY_CONTROL, 2,  KEY_LEFT_CONTROL, KEY_RIGHT_CONTROL  );
	SETEQUATE(KEY_LEFT_ALT, KEY_ALT, 2,  KEY_LEFT_ALT, KEY_RIGHT_ALT  )                 ;
	SETEQUATE(KEY_RIGHT_ALT, KEY_ALT, 2,  KEY_LEFT_ALT, KEY_RIGHT_ALT  )                ;
	SETEQUATE(KEY_PAD_HOME, KEY_HOME, 2,  KEY_PAD_HOME, KEY_GREY_HOME  )                ;
	SETEQUATE(KEY_GREY_HOME, KEY_HOME, 2,  KEY_PAD_HOME, KEY_GREY_HOME  )               ;
	SETEQUATE(KEY_PAD_LEFT, KEY_LEFT, 2,  KEY_PAD_LEFT, KEY_GREY_LEFT  )                ;
	SETEQUATE(KEY_GREY_LEFT, KEY_LEFT, 2,  KEY_PAD_LEFT, KEY_GREY_LEFT  )               ;
	SETEQUATE(KEY_PAD_RIGHT, KEY_RIGHT, 2,  KEY_PAD_RIGHT, KEY_GREY_RIGHT  )            ;
	SETEQUATE(KEY_GREY_RIGHT, KEY_RIGHT, 2,  KEY_PAD_RIGHT, KEY_GREY_RIGHT  )           ;
	SETEQUATE(KEY_PAD_UP, KEY_UP, 2,  KEY_PAD_UP, KEY_GREY_UP  )                        ;
	SETEQUATE(KEY_GREY_UP, KEY_UP, 2,  KEY_PAD_UP, KEY_GREY_UP  )                       ;
	SETEQUATE(KEY_PAD_DOWN, KEY_DOWN, 2,  KEY_PAD_DOWN, KEY_GREY_DOWN  )                ;
	SETEQUATE(KEY_GREY_DOWN, KEY_DOWN, 2,  KEY_PAD_DOWN, KEY_GREY_DOWN  )               ;
	SETEQUATE(KEY_PAD_END, KEY_END, 2,  KEY_PAD_END, KEY_GREY_END  )                    ;
	SETEQUATE(KEY_GREY_END, KEY_END, 2,  KEY_PAD_END, KEY_GREY_END  )                   ;

	SETEQUATE(KEY_PAD_PGDN, KEY_PGDN, 2,  KEY_PAD_PGDN, KEY_GREY_PGDN  )                ;
	SETEQUATE(KEY_GREY_PGDN, KEY_PGDN, 2,  KEY_PAD_PGDN, KEY_GREY_PGDN  )               ;
	SETEQUATE(KEY_PAD_PGUP, KEY_PGUP, 2,  KEY_PAD_PGUP, KEY_GREY_PGUP  )                ;
	SETEQUATE(KEY_GREY_PGUP, KEY_PGUP, 2,  KEY_PAD_PGUP, KEY_GREY_PGUP  )               ;
	SETEQUATE(KEY_PAD_DELETE, KEY_DELETE, 2,  KEY_PAD_DELETE, KEY_GREY_DELETE  )        ;
	SETEQUATE(KEY_GREY_DELETE, KEY_DELETE, 2,  KEY_PAD_DELETE, KEY_GREY_DELETE  )       ;
	SETEQUATE(KEY_PAD_INSERT, KEY_INSERT, 2,  KEY_PAD_INSERT, KEY_GREY_INSERT  )        ;
	SETEQUATE(KEY_GREY_INSERT, KEY_INSERT, 2,  KEY_PAD_INSERT, KEY_GREY_INSERT  )       ;
	SETEQUATE(KEY_PAD_PLUS, KEY_PLUS, 1,  KEY_PAD_PLUS, 0  )                            ;
	SETEQUATE(KEY_PAD_MINUS, KEY_MINUS, 1,  KEY_PAD_MINUS, 0  )                         ;
	SETEQUATE(KEY_PAD_MULT, KEY_MULT, 1,  KEY_PAD_MULT, 0  )                            ;
	SETEQUATE(KEY_PAD_DIV, KEY_DIV, 1,  KEY_PAD_DIV, 0  )                               ;
	SETEQUATE(KEY_PAD_ENTER, KEY_ENTER, 2,  KEY_PAD_ENTER, KEY_NORMAL_ENTER  )          ;
	SETEQUATE(KEY_NORMAL_ENTER, KEY_ENTER, 2,  KEY_PAD_ENTER, KEY_NORMAL_ENTER  )      ;
#endif
}

//FLAGSET( KeyboardState, 512 );
PTHREAD global_pThread;

_32 UnifideSetKey( _32 scancode, LOGICAL bDown )
{
	// results in the common scancode value
	_32 resultcode;
   InitKeyEquates();
#ifdef DEBUG_KEYBOARD_HANDLING
	lprintf( "Keycode %ld is %s", scancode, bDown?"pressed":"released" );
#endif
	if( scancode > 256 )
	{
      Log( WIDE("Scancode is greater than 256 - FAILURE!") );
		return 0;
	}
	if( g.pFocusedPanel )
	{
#ifdef DEBUG_KEYBOARD_HANDLING
		lprintf( "focused panell...." );
#endif
	// always set the natural key's state....
		if( bDown )
		{
			TOGGLEKEY( g.pFocusedPanel, scancode );
			SETKEY( g.pFocusedPanel, scancode );
		}
		else
			CLEARKEY( g.pFocusedPanel, scancode );
	}

	// if the key actually results in a 'common code'
	if( KeyEquates[scancode].common_code )
	{
		resultcode = KeyEquates[scancode].common_code;
		//if( g.pFocusedPanel )
		{
			if( KeyEquates[scancode].numalts == 1 )
			{
			// just mimic the state .. some keys ended up being
			// overly mapped....
				if( KEYPRESSED( g.pFocusedPanel, KeyEquates[scancode].base_codes[0] ) )
					SETKEY( g.pFocusedPanel, KeyEquates[scancode].common_code );
				else
					CLEARKEY( g.pFocusedPanel, KeyEquates[scancode].common_code );

				if( KEYDOWN( g.pFocusedPanel, KeyEquates[scancode].base_codes[0] ) )
					SETTOGGLEKEY( g.pFocusedPanel, KeyEquates[scancode].common_code );
				else
					CLEARTOGGLEKEY( g.pFocusedPanel, KeyEquates[scancode].common_code );
			}
			else if( KeyEquates[scancode].numalts == 2 )
			{
			// if down... check down state of other keys, and down state
			// of THIS key before setting the result key...
            //lprintf( "..." );
				if( bDown )
				{

					if( KEYPRESSED( g.pFocusedPanel, KeyEquates[scancode].base_codes[0] ) ||
						KEYPRESSED( g.pFocusedPanel, KeyEquates[scancode].base_codes[1] ) )
					{
						if( !KEYPRESSED( g.pFocusedPanel, KeyEquates[scancode].common_code ) )
							TOGGLEKEY( g.pFocusedPanel, KeyEquates[scancode].common_code );
					}
				}

				if( !KEYPRESSED( g.pFocusedPanel, KeyEquates[scancode].base_codes[0] ) &&
					!KEYPRESSED( g.pFocusedPanel, KeyEquates[scancode].base_codes[1] ) )
				{
					CLEARKEY( g.pFocusedPanel, KeyEquates[scancode].common_code );
				}
				else // if either and or both are pressed....
				{
					SETKEY( g.pFocusedPanel, KeyEquates[scancode].common_code );
				}
			}
		}
	}
	else
	{
		resultcode = scancode;
	}
	return resultcode;
}

#ifdef _WIN32
// SDL key events go through SDL_KEYDOWN and SDL_KEYUP
// which then invoke the triggers (local) or dispatch the event
int CPROC KeyProcHandler( PTRSZVAL psvUser, _32 key )
{
   PPANEL panel = g.pFocusedPanel;
   lprintf( WIDE("Dispatch key to focused panel... %08x"), key );
	if( panel )
	{
		if( !HandleKeyEvents( g.KeyDefs, key ) )
			if( panel && !HandleKeyEvents( panel->KeyDefs, key ) )
			{
				if( g.pFocusedPanel->KeyMethod )
					return g.pFocusedPanel->KeyMethod( g.pFocusedPanel->psvKey, key );
			}
	}
   return 0;
}
#endif

#include "keymap.h"


#ifdef __arm__
/* Special optimized blit for RGB 8-8-8 --> RGB 5-6-5 */
#define HI 1
#define LO 0
#define RGB888_RGB565(dst, src) { \
	*(_16 *)(dst) = (((*(src))&0x00F80000)>>8)| \
	                   (((*(src))&0x0000FC00)>>5)| \
	                   (((*(src))&0x000000F8)>>3); \
}
#define RGB888_RGB565_TWO(dst, src) { \
	*(_32 *)(dst) = ((((((src)[HI])&0x00F80000)>>8)| \
	                     ((((src)[HI])&0x0000FC00)>>5)| \
	                     ((((src)[HI])&0x000000F8)>>3))<<16)| \
	                     ((((src)[LO])&0x00F80000)>>8)| \
	                     ((((src)[LO])&0x0000FC00)>>5)| \
	                     ((((src)[LO])&0x000000F8)>>3); \
	}

//#define ASM_INLINE
#ifdef ASM_INLINE
static void Blit_RGB888_RGB565( Image pifDest, Image pifSrc, int xofs, int yofs, int width, int height )
{
	asm(
"	@ args = 8, pretend = 0, frame = 16             \n"
"	@ frame_needed = 0, uses_anonymous_args = 0     \n"
"	stmfd	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, lr} \n"
"	sub	sp, sp, #16                               \n"
"	ldr	lr, [r1, #36]                             \n"
"	ldr	r4, [r0, #36]                             \n"
"	ldr	sl, [sp, #52]                             \n"
"	ldr	fp, [r1, #24]                             \n"
"	tst	r2, #1                                    \n"
"	mov	r9, r2                                    \n"
"	mla	r7, fp, r3, r2                            \n"
"	mov	r6, sl, asr #1                            \n"
"	str	lr, [sp, #0]                              \n"
"	str	r4, [sp, #4]                              \n"
"	beq	.MyL241                                     \n"
"	ldr	r5, [sp, #56]                             \n"
"	cmp	r5, #0                                    \n"
"	add	lr, lr, r7, asl #2                        \n"
"	add	r4, r4, r7, asl #1                        \n"
"	beq	.MyL267                                     \n"
"	mov	r0, fp, asl #1                            \n"
"	mov	ip, fp, asl #2                            \n"
".MyL246:                                            \n"
"	ldr	r3, [lr, #0]                              \n"
"	and	r1, r3, #64512                            \n"
"	mov	r2, r1, lsr #5                            \n"
"	and	r1, r3, #16252928                         \n"
"	orr	r6, r2, r1, lsr #8                        \n"
"	and	r3, r3, #248                              \n"
"	orr	r1, r6, r3, lsr #3                        \n"
"	subs	r5, r5, #1                                \n"
"	strh	r1, [r4, #0]	@ movhi                    \n"
"	add	lr, lr, ip                                \n"
"	add	r4, r4, r0                                \n"
"	bne	.MyL246                                     \n"
".MyL267:                                            \n"
"	sub	sl, sl, #1                                \n"
"	add	r9, r9, #1                                \n"
"	add	r7, r7, #1                                \n"
"	mov	r6, sl, asr #1                            \n"
".MyL241:                                            \n"
"	ldmia	sp, {r4, r5}	@ phole ldm                \n"
"	mov	r0, r7, asl #1                            \n"
"	mov	r7, r7, asl #2                            \n"
"	rsb	r3, sl, fp                                \n"
"	add	lr, r4, r7                                \n"
"	add	r4, r5, r0                                \n"
"	ldr	r5, [sp, #56]                             \n"
"	tst	r3, #1                                    \n"
"	addne	r3, r3, #1                                \n"
"	cmp	r5, #0                                    \n"
"	str	r0, [sp, #8]                              \n"
"	str	r7, [sp, #12]                             \n"
"	beq	.MyL269                                     \n"
"	mov	r8, r3, asl #2                            \n"
"	mov	r7, r3, asl #1                            \n"
".MyL257:                                            \n"
"	subs	ip, r6, #0                                \n"
"	beq	.MyL271                                     \n"
".MyL256:                                            \n"
"	ldmia	lr, {r0, r1}	@ phole ldm                \n"
"	and	r3, r1, #64512                            \n"
"	mov	r2, r3, lsr #5                            \n"
"	and	r3, r1, #16252928                         \n"
"	orr	r2, r2, r3, lsr #8                        \n"
"	and	r1, r1, #248                              \n"
"	and	r3, r0, #16252928                         \n"
"	orr	r2, r2, r1, lsr #3                        \n"
"	mov	r1, r3, lsr #8                            \n"
"	orr	r3, r1, r2, asl #16                       \n"
"	and	r2, r0, #64512                            \n"
"	orr	r3, r3, r2, lsr #5                        \n"
"	and	r1, r0, #248                              \n"
"	orr	r2, r3, r1, lsr #3                        \n"
"	subs	ip, ip, #1                                \n"
"	str	r2, [r4], #4                              \n"
"	add	lr, lr, #8                                \n"
"	bne	.MyL256                                     \n"
".MyL271:                                            \n"
"	subs	r5, r5, #1                                \n"
"	add	r4, r4, r7                                \n"
"	add	lr, lr, r8                                \n"
"	bne	.MyL257                                     \n"
".MyL269:                                            \n"
"	tst	sl, #1                                    \n"
"	beq	.MyL273                                     \n"
"	tst	r9, #1                                    \n"
"	beq	.MyL259                                     \n"
".MyL240:                                            \n"
"	add	sp, sp, #16                               \n"
"	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, sl, fp, pc} \n"
".MyL259:                                            \n"
"	ldr	r5, [sp, #56]                             \n"
"	ldr	r4, [sp, #12]                             \n"
"	ldr	r2, [sp, #0]                              \n"
"	ldmib	sp, {r0, ip}	@ phole ldm                \n"
"	cmp	r5, #0                                    \n"
"	add	lr, r2, r4                                \n"
"	add	r4, r0, ip                                \n"
"	beq	.MyL240                                     \n"
"	mov	ip, fp, asl #2                            \n"
"	mov	r0, fp, asl #1         \n"
".MyL265:                         \n"
"	ldr	r3, [lr, #0]           \n"
"	and	r1, r3, #64512         \n"
"	mov	r2, r1, lsr #5         \n"
"	and	r1, r3, #16252928      \n"
"	orr	r2, r2, r1, lsr #8     \n"
"	and	r3, r3, #248           \n"
"	orr	r1, r2, r3, lsr #3     \n"
"	subs	r5, r5, #1             \n"
"	strh	r1, [r4, #0]	@ movhi \n"
"	add	lr, lr, ip             \n"
"	add	r4, r4, r0             \n"
"	bne	.MyL265                  \n"
"	b	.MyL240                     \n"
".MyL273:                         \n"
"	tst	r9, #1                 \n"
"	beq	.MyL240                  \n"
"	b	.MyL259                     \n"
		 "\n"       "\n"::

  );
}
#else
static void Blit_RGB888_RGB565( Image pifDest, Image pifSrc, int xofs, int yofs, int width, int height )
{
	int w2 = (width)>>1;
   int pwidth = pifSrc->width;
	_32 *src = (_32*)pifSrc->image;
	_16 *dst = (_16*)pifDest->image;
   int ofs = xofs + yofs * pifSrc->width;
	register int skip; // = (pifSrc->width - width);

	//src = (_32*)pifSrc->image + ofs;
	//dst = (_16*)pifDest->image + ofs;

	{
		register int x, y;
		if( xofs & 1 )
		{
			src = (_32*)pifSrc->image + ofs;
			dst = (_16*)pifDest->image + ofs;
			for( y = height; y; y-- )
			{
					RGB888_RGB565( dst, src );
					dst += pwidth;
					src += pwidth;
			}
			xofs++;
         width--;
			ofs++; // plus one pixel...
		}
		w2 = (width)>>1;
		skip = (pifSrc->width - width);
		src = (_32*)pifSrc->image + ofs;
		dst = (_16*)pifDest->image + ofs;
		if( skip & 1 )
         skip++;
		for( y = height; y; y-- )
		{
			for( x = w2; x; x-- )
			{
				RGB888_RGB565_TWO( dst, src);
				dst+=2;
            src+=2;
			}
			dst += skip;
			src += skip;
		}

		if( ((width & 1) && !(xofs&1)) ||
			(!(width & 1) && (xofs&1)) )
		{
			src = (_32*)pifSrc->image + ofs;
			dst = (_16*)pifDest->image + ofs;
			for( y = height; y; y-- )
			{
				RGB888_RGB565( dst, src );
				dst += pwidth;
				src += pwidth;
			}
		}
	}
}
#endif
#endif

static void CloseAllDisplays( PPANEL pDisp );
void cleanup( void )
{
	CloseAllDisplays( g.pRootPanel );
}
//void cleanup( void );


void ThreadDoUpdateRect( void )
{
   THREAD_ID prior = 0;
	S_32 x, y;
	_32 w, h;
	while( EnterCriticalSecNoWait( &g.csUpdating, &prior ) <= 0 )
	{
	// update code may be attempting to post an update region
	// therefore we have to unlock this section to allow the g.csUpdating to be avaialble...
      //lprintf( "Thread update - updating is locked, so leave thread..." );
		LeaveCriticalSec( &csThreadMessage );
      // this is what the timer code does... might not want this... might lose the wake event...
		//if( d == 0 )
      //   WakeableSleep( SLEEP_FOREVER );
		EnterCriticalSec( &csThreadMessage );
      //lprintf( "Thread update - updating is locked, so lock thread..." );
      //DebugBreak();
	}
	////lprintf( WIDE("THREAD_DO_UPDATE_RECT") );
	//EnterCriticalSec( &update_rect.cs );
	//if( !update_rect.flags.bHasContent )
	{
		// oops - somehow double queued the message
						// but already handled the update...
                     //lprintf( WIDE("Double queued update message... already flushed.") );
							//LeaveCriticalSec( &update_rect.cs );
							//break;
						}
#ifdef INTERNAL_BUFFER
						// SoftSurface is the REAL surface... (which the name would
						// not imply) whereas the image at g.pRootPanel is just
						// an internal memory buffer... so update g.Root to g.Soft
						// and then tell SDL to update it's buffer to the screen...
						// would have to be display blot, not jsut blot
						// so that all portions which are clipped are omitted.
						//lprintf( WIDE("blotting virtual buffer to real buffer before update... %d,%d %d,%d")
					//		, update_rect.x, update_rect.y, update_rect.width, update_rect.height );
#ifdef MOUSE_CURSOR_DEBUG
						lprintf( WIDE("Rendering mosue wihtout update...") );
#endif
						//RenderMouse( &update_rect, FALSE );
#else
#endif
#ifdef DEBUG_DIRTY_RECT
						lprintf( WIDE("SDL Update rectangle %") _32fs WIDE(" %") _32fs WIDE(" %") _32fs WIDE(" %") _32fs WIDE("")
								 , thread_messages[thread_head].data.update_rect.x
								 , thread_messages[thread_head].data.update_rect.y
								 , thread_messages[thread_head].data.update_rect.width
								 , thread_messages[thread_head].data.update_rect.height );
#endif
#ifdef DEBUG_DIRTY_RECT
						{
							SPACEPOINT min, max;
							min[0] = thread_messages[thread_head].data.update_rect.x;
							min[1] = thread_messages[thread_head].data.update_rect.y;
							max[0] = thread_messages[thread_head].data.update_rect.width
									 + thread_messages[thread_head].data.update_rect.x - 1;
							max[1] = thread_messages[thread_head].data.update_rect.height
									 + thread_messages[thread_head].data.update_rect.y - 1;
							//lprintf( WIDE("wtf?!") );
							DrawSpace( NULL, min, max, SetAlpha( BASE_COLOR_RED, 32 ) );
						}
#endif
                  /*
						g.ImageInterface->_BlotImageSizedEx( g.RealSurface, g.SoftSurface
																	  , thread_messages[thread_head].data.update_rect.x
																	  , thread_messages[thread_head].data.update_rect.y
																	  , thread_messages[thread_head].data.update_rect.x
																	  , thread_messages[thread_head].data.update_rect.y
																	  , thread_messages[thread_head].data.update_rect.width
																	  , thread_messages[thread_head].data.update_rect.height
																	  , 0, BLOT_COPY );
																	  */
						if( ( x = thread_messages[thread_head].data.update_rect.x ) < 0 )
						{
                     thread_messages[thread_head].data.update_rect.width += thread_messages[thread_head].data.update_rect.x;
							thread_messages[thread_head].data.update_rect.x = 0;
						}
						if( ( y = thread_messages[thread_head].data.update_rect.y ) < 0 )
						{
                     thread_messages[thread_head].data.update_rect.height += thread_messages[thread_head].data.update_rect.y;
							thread_messages[thread_head].data.update_rect.y = 0;
						}
						if( thread_messages[thread_head].data.update_rect.x
						  + ( w = thread_messages[thread_head].data.update_rect.width )
							> g.width )
						{
                     thread_messages[thread_head].data.update_rect.width = g.width - thread_messages[thread_head].data.update_rect.x;
						}
						if( thread_messages[thread_head].data.update_rect.y
						  + ( h = thread_messages[thread_head].data.update_rect.height )
							> g.height )
						{
                     thread_messages[thread_head].data.update_rect.height = g.height - thread_messages[thread_head].data.update_rect.y;
						}
//#if 0
						if( g.sprites )
						{
							INDEX idx;
							PSPRITE_METHOD psm;

							LIST_FORALL( g.sprites, idx, PSPRITE_METHOD, psm )
							{
								if( psm->RenderSprites )
								{
									PRENDERER r;
									for( r = psm->renderer; r; r = r->parent )
									{
										x -= r->common.x;
										y -= r->common.y;
									}
									BlotImage( psm->original_surface, psm->renderer->common.RealImage
												, 0, 0 );
								// if I exported the PSPRITE_METHOD structure to the image library
								// then it could itself short circuit the drawing...
									psm->RenderSprites( psm->psv, psm->renderer, x, y, w, h );
								}
							}
						}
//#endif
#ifdef __LINUX__
# ifdef __ARM__
                  // happen
						BlotImageSizedTo( g.PhysicalSurface
											 , g.RealSurface
											 , thread_messages[thread_head].data.update_rect.x
											 , thread_messages[thread_head].data.update_rect.y
											 , thread_messages[thread_head].data.update_rect.x
											 , thread_messages[thread_head].data.update_rect.y
											 , thread_messages[thread_head].data.update_rect.width
											 , thread_messages[thread_head].data.update_rect.height );
                  /*
						Blit_RGB888_RGB565( g.RealSurface
												, g.SoftSurface
												, thread_messages[thread_head].data.update_rect.x
												, thread_messages[thread_head].data.update_rect.y
												, thread_messages[thread_head].data.update_rect.width
												, thread_messages[thread_head].data.update_rect.height );
                                    */
# elif !defined( __NO_SDL__ )
						SDL_UpdateRect( g.surface
										  , thread_messages[thread_head].data.update_rect.x
										  , thread_messages[thread_head].data.update_rect.y
										  , thread_messages[thread_head].data.update_rect.width
										  , thread_messages[thread_head].data.update_rect.height );
# else
										  // without SDL to raw frame buffer, updates from softsurface to real surface
                  // happen
						BlotImageSizedTo( g.PhysicalSurface
											 , g.RealSurface
											 , thread_messages[thread_head].data.update_rect.x
											 , thread_messages[thread_head].data.update_rect.y
											 , thread_messages[thread_head].data.update_rect.x
											 , thread_messages[thread_head].data.update_rect.y
											 , thread_messages[thread_head].data.update_rect.width
											 , thread_messages[thread_head].data.update_rect.height );
											 //printf( "Update surface - thread message done...\n" );
# endif
#else // WIN32 ?
                  //WriteToWindow();
						BlotImageSizedTo( g.RealSurface
											 , g.SoftSurface
											 , thread_messages[thread_head].data.update_rect.x
											 , thread_messages[thread_head].data.update_rect.y
											 , thread_messages[thread_head].data.update_rect.x
											 , thread_messages[thread_head].data.update_rect.y
											 , thread_messages[thread_head].data.update_rect.width
											 , thread_messages[thread_head].data.update_rect.height );
#endif
                  //Log( WIDE("Done with update... there should have been messages...") );
						//Log( WIDE("SDL Update rectangle done") );
                  //update_rect.flags.bHasContent = 0;
                  //LeaveCriticalSec( &update_rect.cs );
   LeaveCriticalSec( &g.csUpdating );
}

// this routine is used by __arm__ to post thread events
// these are also the same sort of events that we use SDL_UserEvents
// could later retrofit the posting of SDL_USEREVENT to just use local message service.
int CPROC LocalEventHandler(_32 MsgID, _32*params, _32 paramlen)
{
#ifdef LOG_UPDATE_PROCESSING
	int updates = 0;
#endif
	switch( MsgID )
	{
	case MSG_ThreadEventPost:
		//lprintf( WIDE("USer event...") );
		EnterCriticalSec( &csThreadMessage );
		//lprintf( WIDE("And section is available..") );
		while( thread_head != thread_tail )
		{
			_32 new_head;
			switch( thread_messages[thread_head].ID )
			{
			case THREAD_DO_SETMOUSEPOS:
#ifdef _WIN32
				SetCursorPos( thread_messages[thread_head].data.mousepos.x
								, thread_messages[thread_head].data.mousepos.y );
#else
#ifndef __NO_SDL__
// we're the only sort of mouse that exists for pure FrameBuffer.
				SDL_WarpMouse( thread_messages[thread_head].data.mousepos.x
								 , thread_messages[thread_head].data.mousepos.y );
#endif
#endif
				break;
			case THREAD_DO_UPDATE_RECT:
			/* this shoudl perhaps just collect a region...*/
#ifdef LOG_UPDATE_PROCESSING
				lprintf( "Doing an update region..." );
				updates++;
#endif
#if defined( __NO_SDL__ )
				ThreadDoUpdateRect();
#else
				ThreadDoUpdateRect();
#endif
				//Log( WIDE("Done with update... there should have been messages...") );
				//Log( WIDE("SDL Update rectangle done") );
				//update_rect.flags.bHasContent = 0;
				//LeaveCriticalSec( &update_rect.cs );
				break;
			case THREAD_DO_EXIT:
#if !defined( __NO_SDL__ )
				lprintf( WIDE("Get SDL_Quit") );
				SDL_Quit();
#endif
				LeaveCriticalSec( &csThreadMessage );
				return -1;
			case THREAD_DO_NOTHING:
			default:
				break;
			}
			new_head = thread_head + 1;
			if( new_head >= MAX_THREAD_MESSAGES )
				new_head -= MAX_THREAD_MESSAGES;
			thread_head = new_head;
			// Leave this a moment... now there is space free to enque the message...
			LeaveCriticalSec( &csThreadMessage );
			/*
			 * --- allow posters to post HERE if they were waiting, they would be awoken --
			 */
			EnterCriticalSec( &csThreadMessage );
		}
		/* update_all_regions... */
#ifdef LOG_UPDATE_PROCESSING
		lprintf( "did %d update regions...", updates );
#endif
		LeaveCriticalSec( &csThreadMessage );

      return EVENT_HANDLED;
	}
   return EVENT_HANDLED;
}

#ifndef __arm__
RENDER_PROC( int, ProcessDisplayMessages )( PTRSZVAL idleproc )
{
#ifdef __SDL__
	struct {
      _32 b;
	} mouse_data;
	static _32 mouse_pending;
	SDL_Event event;
	if( !IsThisThread( global_pThread ) )
	{
      //lprintf( WIDE("global thread is not set to me!") );
		return -1;
	}
   //lprintf( WIDE("Allowed thread is getting a message...") );
	if( !idleproc )
	{
		if( !SDL_PollEvent( &event ) )
		{
         return 0;
		}
	}
	while( (idleproc && SDL_PollEvent( &event )) ||
			 (!idleproc && SDL_WaitEvent( &event )) )
	{
#if defined( MOUSE_EVENT_DEBUG )
		lprintf( WIDE("Some event...") );
#endif
		do
		{
			switch( event.type )
			{
			case SDL_USEREVENT:
            LocalEventHandler( MSG_ThreadEventPost, NULL, 0 );
				break;
			case SDL_ACTIVEEVENT:
				if( event.active.gain )
				{
#ifdef DEBUG_FOCUS_EVENTS
					lprintf( WIDE("Gaining focus...") );
#endif
					LoseFocusHandler( 0, (PRENDERER)0 );
				}
				else
				{
#ifdef DEBUG_FOCUS_EVENTS
					lprintf( WIDE("Losing focus...") );
#endif
					LoseFocusHandler( 0, (PRENDERER)1 );
				}
            break;
			case SDL_VIDEOEXPOSE:
				EnterCriticalSec( &csThreadMessage );
				DrawAllPanels();
				ThreadDoUpdateRect();
				LeaveCriticalSec( &csThreadMessage );
            break;
			case SDL_NOEVENT: // no event?
				break;
			default:
				Log1( WIDE("Unknown SDL Event received: %d"), event.type );
				break;
			case SDL_KEYDOWN:
#ifdef DEBUG_KEYBOARD_HANDLING
			Log6( WIDE("Key Down: %d %d %d %d %d %d")
				 , event.key.which
				 , event.key.state
				 , event.key.keysym.scancode
				 , event.key.keysym.unicode
				 , event.key.keysym.sym
				 , event.key.keysym.mod );
				lprintf( "key is %s and %s"
						 , keymap[event.key.keysym.sym].keyname
						 , keymap[event.key.keysym.sym].othername );
#endif
			{
				_32 key;
				int keymod, scancode;
				key = UnifideSetKey( scancode = keymap[event.key.keysym.sym].scancode, TRUE );
				key |= scancode << 16;
				key |= KEY_PRESSED;
				keymod = 0;
				//if( g.pFocusedPanel )
				{
					if( KEYPRESSED( g.pFocusedPanel, KEY_SHIFT ) )
					{
#ifdef DEBUG_KEYBOARD_HANDLING
						lprintf( "Shift" );
#endif
						keymod |= KEY_MOD_SHIFT;
						key |= KEY_SHIFT_DOWN;
					}
					if( KEYPRESSED( g.pFocusedPanel, KEY_CONTROL ) )
					{
#ifdef DEBUG_KEYBOARD_HANDLING
						lprintf( "Control" );
#endif
						keymod |= KEY_MOD_CTRL;
						key |= KEY_CONTROL_DOWN;
					}
					if( KEYPRESSED( g.pFocusedPanel, KEY_ALT ) )
					{
#ifdef DEBUG_KEYBOARD_HANDLING
						lprintf( "Alt" );
#endif
						keymod |= KEY_MOD_ALT;
						key |= KEY_ALT_DOWN;
					}
							  // SDL ends up issuing a down on lock, and an up on unlock
					if( KEYPRESSED( g.pFocusedPanel, KEY_CAPS_LOCK ) )
					{
						key |= KEY_ALPHA_LOCK_ON;
					}
					if( KEYPRESSED( g.pFocusedPanel, KEY_NUM_LOCK ) )
						key |= KEY_NUM_LOCK_ON;
				}
				{
					PPANEL panel = g.pFocusedPanel;
#ifdef DEBUG_KEYBOARD_HANDLING
					lprintf( "panel is %p", panel );
#endif
					// event is probably a remote-dispatched
					// widget... and no confirmation message
					// do don't add use...?
					//AddUse( panel );
					if( !HandleKeyEvents( g.KeyDefs, key ) )
						if( panel && !HandleKeyEvents( panel->KeyDefs, key ) )
						{
							if( panel->KeyMethod )
							{
#ifdef DEBUG_KEYBOARD_HANDLING
								lprintf( WIDE("Dispatch to focused keymethod %08") _32fx WIDE(""), key );
#endif
								panel->KeyMethod( panel->psvKey, key );
							}
							else if( panel->MouseMethod )
							{
#ifdef DEBUG_KEYBOARD_HANDLING
								lprintf( WIDE("Dispatch to focused mousemethod MK_OBUTTON %08") _32fx WIDE(""), key );
#endif
								panel->MouseMethod( panel->psvMouse
														, mouse_x
														, mouse_y
														, mouse_buttons | MK_OBUTTON ); // other button.
							}
						}
					//g.pFocusedPanel = DeleteUse( panel );
					//RenderDirtyRegions();
				}
			}

         break;
			case SDL_KEYUP:
#ifdef DEBUG_KEYBOARD_HANDLING

			Log6( WIDE("Key up  : %d %d %d %d %d %d")
				 , event.key.which
				 , event.key.state
				 , event.key.keysym.scancode
				 , event.key.keysym.unicode
				 , event.key.keysym.sym
				 , event.key.keysym.mod );
				lprintf( "key is %s and %s"
						 , keymap[event.key.keysym.sym].keyname
						 , keymap[event.key.keysym.sym].othername );
#endif
			{
				_32 key;
				PPANEL panel = g.pFocusedPanel;
            //lprintf( "panel is %p", panel );
				key = UnifideSetKey( keymap[event.key.keysym.sym].scancode, FALSE );
				key |= event.key.keysym.sym << 16;

				//if( panel )
				{
					if( KEYPRESSED( panel, KEY_SHIFT ) )
						key |= KEY_SHIFT_DOWN;
					if( KEYPRESSED( panel, KEY_CONTROL ) )
						key |= KEY_CONTROL_DOWN;
					if( KEYPRESSED( panel, KEY_ALT ) )
						key |= KEY_ALT_DOWN;
					if( KEYPRESSED( panel, KEY_CAPS_LOCK ) )
						key |= KEY_ALPHA_LOCK_ON;
					if( KEYPRESSED( panel, KEY_NUM_LOCK ) )
						key |= KEY_NUM_LOCK_ON;
							  //key |= event.key.keysym.sym << 8;
					if( panel )
					{
                  // key up may ahve a seperate event from key down...
						if( !HandleKeyEvents( g.KeyDefs, key ) )
							if( panel && !HandleKeyEvents( panel->KeyDefs, key ) )
							{
								if( panel->KeyMethod )
								{
									panel->KeyMethod( panel->psvKey, key );
								}
								else if( panel->MouseMethod )
								{
									AddUse( panel );
									panel->MouseMethod( panel->psvMouse
															, mouse_x
															, mouse_y
															, mouse_buttons | MK_OBUTTON ); // other button.
									panel = DeleteUse( panel );
								}
							}
					}
				}
			}

			break;
		case SDL_QUIT:
			lprintf( WIDE("Get SDL_Quit - SDL is quitting....") );
			CloseAllDisplays( g.pRootPanel );
			return -1;
			break;
		case SDL_MOUSEMOTION:
			mouse_x = event.motion.x;
			mouse_y = event.motion.y;
			mouse_data.b = mouse_buttons;
			mouse_pending++;
#ifdef MOUSE_EVENT_DEBUG
			Log3( WIDE("Did mouse event: %ld %ld %08lx"), mouse_x, mouse_y, mouse_buttons );
#endif
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			g._last_buttons = mouse_buttons;
			if( event.button.button == SDL_BUTTON_LEFT )
				event.button.button = MK_LBUTTON;
			else if( event.button.button == SDL_BUTTON_RIGHT )
				event.button.button = MK_RBUTTON;
			else if( event.button.button == SDL_BUTTON_MIDDLE )
				event.button.button = MK_MBUTTON;

			if( event.button.type == SDL_MOUSEBUTTONUP )
				mouse_buttons &= ~event.button.button;
			else
				mouse_buttons |= event.button.button;
         // there was definatly a change - silly to check for it.
			if( mouse_pending )
			{
            //Log1( WIDE("Forcing mouse event (button) with %ld events"), mouse_pending );
				Mouse( 0
					  , mouse_x
					  , mouse_y
					  , mouse_data.b );
            mouse_pending = 0;
			}
			mouse_x = event.button.x;
			mouse_y = event.button.y;
			// keep button state - so we can check for delta buttons
         // while collecting events.
         mouse_data.b = mouse_buttons;
			mouse_pending++;
			//Log3( WIDE("Did mouse event: %ld %ld %08lx"), mouse_x, mouse_y, mouse_buttons );
			break;
			}
		} while( SDL_PollEvent( &event ) );
		if( mouse_pending )
		{
			//Log1( WIDE("Doing mouse with %ld events"), mouse_pending );
			Mouse( 0, mouse_x, mouse_y, mouse_buttons );
			mouse_pending = 0;
		}
	}
	return 1;
#else // not __SDL__ (therefore windwos?)
	if( g.RenderInterface
	  && g.RenderInterface->_ProcessDisplayMessages )
		return g.RenderInterface->_ProcessDisplayMessages();
   return 0;
#endif
}
#endif

//---------------------------------------------------------------------------

void GenerateMouseRaw( S_32 x, S_32 y, _32 b )
{
	Mouse( 0, x, y, b );
}

//---------------------------------------------------------------------------

void GenerateMouseDeltaRaw( S_32 x, S_32 y, _32 b )
{
	static struct {
		S_32 x;
		S_32 y;
		S_32 z;
      S_32 w;
	} realmouse;
	if( b & 0x100 )
	{
		realmouse.w++;
      b &= ~0x100;
	}
	if( b & 0x200 )
	{
		realmouse.w--;
      b &= ~0x200;
	}
	realmouse.x += x;
	if( realmouse.x < 0 )
		realmouse.x = 0;
	if( realmouse.x >= g.width )
		realmouse.x = g.width - 1;

	realmouse.y += y;
	if( realmouse.y < 0 )
		realmouse.y = 0;
	if( realmouse.y >= g.height )
		realmouse.y = g.height - 1;
   //printf( "mouse is %d,%d %08x\n", realmouse.x, realmouse.y, b );
	Mouse( 0, realmouse.x, realmouse.y, b );
}

//---------------------------------------------------------------------------

#ifndef __arm__
static PTRSZVAL CPROC ProcessMessageThread( PTHREAD pThread )
{
	int n;
   //lprintf( WIDE("------------- STARTING THREAD %p"), pThread );
   global_pThread = pThread;
   while( ( n = ProcessDisplayMessages( 0 ) ) >= 0 )
	{
	}
	global_pThread = NULL;
   return 0;
}
#endif

//---------------------------------------------------------------------------

#undef SetSize
static PTRSZVAL CPROC SetSize( PTRSZVAL psv, arg_list args  )
{
	g.width = my_va_arg( args, _64 );
	g.height = my_va_arg( args, _64 );
	lprintf( WIDE("Setting size to %d by %d")
			 , g.width, g.height );
   return psv;
}

//---------------------------------------------------------------------------

static PTRSZVAL CPROC SetOffset( PTRSZVAL psv, arg_list args  )
{
	g.ofs_x = my_va_arg( args, _64 );
	g.ofs_y = my_va_arg( args, _64 );
   lprintf( WIDE("Setting offset at %d, %d"), g.ofs_x, g.ofs_y );
   return psv;
}

//---------------------------------------------------------------------------
#if 0
static PTRSZVAL CPROC SetDepth( PTRSZVAL psv, arg_list args )
{
	PARAM( args, _64, depth );
   lprintf( WIDE("Should set the depth to something...%d"), depth );
   return psv;
}
#endif
//---------------------------------------------------------------------------
#ifdef INTERNAL_BUFFER

static PTRSZVAL CPROC SetSoftCursorAlpha( PTRSZVAL psv, arg_list args )
{
   S_64 alpha = my_va_arg( args, S_64 );
   if( alpha > 0 )
		g.SoftCursorAlpha = ALPHA_TRANSPARENT_INVERT + alpha;
	else
      g.SoftCursorAlpha = ALPHA_TRANSPARENT - alpha;
   return psv;

}
//---------------------------------------------------------------------------
static PTRSZVAL CPROC SetSoftCursorImage( PTRSZVAL psv, arg_list args )
{
   lprintf( WIDE("Setting soft image... %p(%s) %p"), (POINTER)psv, (CTEXTSTR)psv, args );
	{
   char *name = my_va_arg( args, char* );
	char *path = (char*)psv;
	char tmp[256];
   lprintf( WIDE("setting cursor image....") );
	sprintf( tmp, WIDE("%s%s%s"), path?path:"", path&&path[0]?"/":"",name );
   Log1( WIDE("Loading [%s]"), tmp );
	g.SoftCursor = g.ImageInterface->_LoadImageFileEx( tmp DBG_SRC );
	}
   return psv;
}

//---------------------------------------------------------------------------

static PTRSZVAL CPROC SetSoftCursorBehavior(PTRSZVAL psv, arg_list args )
{
   char *option = my_va_arg( args, char *);
	Log1( WIDE("Cursor behavior: \'%s\'"), option );
	if( !stricmp( option, WIDE("down") ) )
		g.flags.soft_cursor_down = 1;
	if( !stricmp( option, WIDE("up") ) )
		g.flags.soft_cursor_up = 1;
	if( !stricmp( option, WIDE("ondown") ) )
		g.flags.soft_cursor_on_down = 1;
	if( !stricmp( option, WIDE("onup") ) )
		g.flags.soft_cursor_on_up = 1;
	if( !stricmp( option, WIDE("always") ) )
		g.flags.soft_cursor_always = 1;
   return psv;
}
#endif

static PTRSZVAL CPROC SetEloMin( PTRSZVAL psv, arg_list args )
{
#ifdef __LINUX__
	char val[12];
	_64 x = my_va_arg( args, _64 );
	_64 y = my_va_arg( args, _64 );
	snprintf( val, 12, WIDE("%Ld"), x );
   setenv( WIDE("SDL_ELO_MIN_X"), val, TRUE );
	snprintf( val, 12, WIDE("%Ld"), y );
   setenv( WIDE("SDL_ELO_MIN_Y"), val, TRUE );
#endif
   return psv;
}

static PTRSZVAL CPROC SetEloMax( PTRSZVAL psv, arg_list args )
{
#ifdef __LINUX__
	char val[12];
	_64 x = my_va_arg( args, _64 );
	_64 y = my_va_arg( args, _64 );
	snprintf( val, 12, WIDE("%Ld"), x );
   setenv( WIDE("SDL_ELO_MAX_X"), val, TRUE );
	snprintf( val, 12, WIDE("%Ld"), y );
   setenv( WIDE("SDL_ELO_MAX_Y"), val, TRUE );
#endif
   return psv;
}

int IsTouchDisplay( void )
{
   return g.flags.touch_display;
}

PTRSZVAL CPROC SetIsTouchScreen( PTRSZVAL psv, arg_list args )
{
   LOGICAL yes = my_va_arg( args, LOGICAL );
	g.flags.touch_display = yes;
   return psv;
}

PTRSZVAL CPROC SetMouseDriver( PTRSZVAL psv, arg_list args )
{
#ifdef __LINUX__
   char *name = my_va_arg( args, char* );
	Log1( WIDE("Setting Mouse Driver: %s"), name );
	setenv( WIDE("SDL_MOUSEDRV"), name, TRUE );
#endif
   return psv;
}

PTRSZVAL SetMouseDevice( PTRSZVAL psv, arg_list args )
{
#ifdef __LINUX__
   char *name = my_va_arg( args, char *);
	Log1( WIDE("Setting Mouse Device: %s"), name );
	//setenv( WIDE("SDL_MOUSEDEV"), name, TRUE );
#endif
   return psv;
}

//---------------------------------------------------------------------------

#ifdef INTERNAL_BUFFER
PTRSZVAL CPROC SetCursorHotspot( PTRSZVAL psv, arg_list args )
{
	S_64 x = my_va_arg( args, S_64 );
	S_64 y = my_va_arg( args, S_64 );
   g.SoftCursorHotX = (S_32)x;
	g.SoftCursorHotY = (S_32)y;
   return psv;
}
#endif

void DrawPanelBackdrop( PPANEL pPanel )
{
   if( pPanel->common.flags.backdrop_color )
   {
      if( pPanel->common.flags.alpha )
      {
      }
      else
      {
      }
   }
   else if( pPanel->common.flags.backdrop_image )
   {
      if( pPanel->common.flags.backdrop_tiled )
      {
         if( pPanel->common.flags.alpha )
         {
         }
         else
         {
         }
      }
      else
      {
         if( pPanel->common.flags.alpha )
         {
         }
         else
         {
         }
      }
   }
   else if( pPanel->common.flags.backdrop_custom )
   {
      if( pPanel->common.background.custom.DrawBackdrop )
         pPanel->common.background.custom.DrawBackdrop
                  ( pPanel->common.background.custom.psvUserDraw
                  , pPanel );
   }
}


//---------------------------------------------------------------------------

static  void DeinitMemory( void )
{
	//if( g )
	//{
		//Release( (POINTER)pDisplayHeap );
      //g = 0;
	//}
}

//---------------------------------------------------------------------------

#define MEMORY_BASE 0xA80000

LOGICAL InitMemory( void )
{
	_32 dwSize = 0;
	//Log( WIDE("Init Memory!") );
	if( !pDisplayHeap)
	{
		dwSize = 0x10000;
		pDisplayHeap = (PMEM)HeapAllocate( 0, dwSize );
		MemSet( (POINTER)pDisplayHeap, 0, dwSize );
						 // always reset the momeory - for now..
		if( dwSize )  // did allocate something of some size...
		{
			lprintf( WIDE("init heap?") );
			InitHeap( pDisplayHeap, dwSize );
		//g.pNameSpace = Allocate( 8192 );
		//g.pNameSpace[0] = g.pNameSpace[1] = 0;
		}
	}

	//SetInterfaceConfigFile( WIDE("DisplayService.conf") );
   if( !g.ImageInterface )
	{
#ifdef __STATIC__
#define NAME "bag.real_image.plugin.static"
#else
#define NAME "bag.real_image.plugin"
#endif
	//g.ImageInterface = GetInterface( WIDE("real_image") );
//#ifndef __LINUX__
//#undef GetImageInterface
//		g.ImageInterface = GetImageInterface();
//#else
		//((PIMAGE_INTERFACE(*)(void))LoadPrivateFunctionEx( NAME, WIDE("GetImageInterface") DBG_SRC ))();
		g.ImageInterface = (PIMAGE_INTERFACE)GetInterface( WIDE("real_image") );
//#endif
	}
	if( !g.RenderInterface )
	{
#ifdef __LINUX__
      extern RENDER_INTERFACE VideoInterface;
		g.RenderInterface = &VideoInterface;
#else
//#undef GetDisplayInterface
		{
         //__declspec(dllimport) PRENDER_INTERFACE CPROC GetDisplayInterface( void );
			g.RenderInterface = (PRENDER_INTERFACE)GetInterface( "video" );
		}
#endif
	}
   if( !g.KeyDefs )
		g.KeyDefs = CreateKeyBinder();
#ifdef __NO_SDL__
	/*
	 * At this point, we should be able to get the framebuffer address
	 * We can also init local service
	 */
	//g.dwMsgBase = LoadService( NULL, LocalEventHandler );
	if( g.dwMsgBase == INVALID_INDEX )
	{
      lprintf( "Failed to register local event, I am sad." );
		return FALSE;
	}
#endif
	return TRUE;
}

//---------------------------------------------------------------------------

void ComputeMinMax( Image image, PSPACEPOINT min, PSPACEPOINT max )
{
   SPACEPOINT origin;
   Image pPanel = image;
   origin[0] = 0;
   origin[1] = 0;
   for( pPanel = image; 
   	  pPanel && pPanel->pParent; 
   	  pPanel = pPanel->pParent )
   {
      // cannot be done with +=  (float) += int is not supported.
      origin[0] = origin[0] + pPanel->x;
      origin[1] = origin[1] + pPanel->y;
   }
   max[0] = min[0] + image->width;
   max[1] = min[1] + image->height;
}

//---------------------------------------------------------------------------

void PanelComputeMinMax( Image panel, PSPACEPOINT min, PSPACEPOINT max )
{
   SPACEPOINT origin;
   Image pPanel;
   origin[0] = 0;
   origin[1] = 0;
   for( pPanel = panel; 
   	  pPanel && pPanel->pParent; 
   	  pPanel = pPanel->pParent )
   {
      // cannot be done with +=  (float) += int is not supported.
      origin[0] = origin[0] + pPanel->x;
      origin[1] = origin[1] + pPanel->y;
   }
   max[0] = min[0] + panel->width;
   max[1] = min[1] + panel->height;
}

//---------------------------------------------------------------------------

RENDER_NAMESPACE_END
DISPLAY_NAMESPACE
void BuildSpaceTreeEx( PPANEL dirty_panel )
#define BuildSpaceTree() BuildSpaceTreeEx( NULL )
{
   PPANEL _panel;
	PPANEL panel;
#ifdef REDRAW_DEBUG
	lprintf( WIDE("building space tree") );
#endif
   EnterCriticalSec( &g.csSpaceRoot );
	//panel = g.pTopMost;
	DeleteSpaceTree( &g.pSpaceRoot );
	panel = g.pTopMost;
	while( panel )
	{
		SPACEPOINT min, max;
		// min and max are INCLUSIVE of
		// all data.
		if( !panel->common.flags.invisible )
		{
			min[0] = panel->common.x;
			min[1] = panel->common.y;
			max[0] = min[0] + panel->common.width - 1;
			max[1] = min[1] + panel->common.height - 1;
			if( min[0] < 0 )
				min[0] = 0;
			if( min[1] < 0 )
				min[1] = 0;
			if( max[0] >= g.width )
				max[1] = g.width - 1;
			if( max[1] >= g.height )
				max[1] = g.height - 1;
			// only add tracked spaces which are on the screen.
			// nothing that overflows the edge will count.
			// this will provide auto clipping for image functions done through
						 // thirs.
#ifdef DEBUG_DIRTY_RECT
			lprintf( WIDE("Node is %p (%") _32fs WIDE(",%") _32fs WIDE(")-(%") _32f WIDE(",%") _32f WIDE(")"), panel, min[0], min[1], max[0], max[1] );
#endif
			{
				PSPACENODE node = AddSpaceNode( &g.pSpaceRoot, panel, min, max );
				if( ( panel == dirty_panel ) || panel->common.flags.initial )
				{
#ifdef REDRAW_DEBUG
					lprintf( "DIRTY *** to start" );
#endif
					MarkNodeDirty( node, NULL );
				}

			}
		}
      _panel = panel;
		panel = panel->above; // the root panel will NEVER be above anything.
	}
   LeaveCriticalSec( &g.csSpaceRoot );
	//BrowseSpaceTree( g.pSpaceRoot, NULL );
   // clears background/unused spaces...
	//DrawSpaces2();
   //Log( WIDE("Drawn spaces") );
}
DISPLAY_NAMESPACE_END
RENDER_NAMESPACE

//---------------------------------------------------------------------------

void RealHideDisplay( PPANEL display, LOGICAL bHide )
{
	if( display->common.flags.initial  && !bHide )
	{
      // this works as a show window also...
		UpdateDisplay( display );
	}
	else
	{
		display->common.flags.invisible = bHide;
		BuildSpaceTree();
		DispatchDrawEvents( NULL, display, FALSE );
	}
}

//---------------------------------------------------------------------------

RENDER_PROC( void, HideDisplay )( PPANEL display )
{
	RealHideDisplay( display, TRUE );
}

//---------------------------------------------------------------------------

RENDER_PROC( void, RestoreDisplay )( PPANEL display )
{
 	RealHideDisplay( display, FALSE );
}

//---------------------------------------------------------------------------

void AddPanelToStackEx( PPANEL panel, PPANEL parent, LOGICAL update )
{
   if( !panel )
      return;
   if( !parent )
		parent = g.pTopMost;
	if( !parent )
      return; // not stackable...
	if( panel == parent )
	{
		Log( WIDE("Putting panel above itself!?") );
		return;
	}
   // should add at the topmost of 
   // this parent... but for now this will be okay.
   panel->above = parent;
   if( ( panel->below = parent->below ) )
      panel->below->above = panel;
   parent->below = panel;
   if( g.pTopMost == parent )
		g.pTopMost = panel;
	if( update )
		BuildSpaceTree();
}

//---------------------------------------------------------------------------

RENDER_PROC( void, AddPanelToStack )( PPANEL panel, PPANEL parent  )
{
	AddPanelToStackEx( panel, parent, TRUE );
}

//---------------------------------------------------------------------------

PPANEL GrabPanelFromStackEx( PPANEL panel, LOGICAL update )
{
   if( panel->above )
      panel->above->below = panel->below;
   if( panel->below ) // if it's below something
      panel->below->above = panel->above;
   else // otherwise it was the topmose display
      g.pTopMost = panel->above; // set topmost to what we're above.
   panel->above = NULL;
   panel->below = NULL;
	if( update )
	{
      //Log1( WIDE("New tree should not have %p in it."), panel );
		BuildSpaceTree();
	}
   return panel; // useful to for stacking grab and put above.
}

//---------------------------------------------------------------------------

RENDER_PROC( PPANEL, GrabPanelFromStack )( PPANEL panel )
{
	return GrabPanelFromStackEx( panel, TRUE );
}

//---------------------------------------------------------------------------
void PutDisplayAboveEx( PPANEL panel, PPANEL parent, LOGICAL bUpdate )
{
	GrabPanelFromStackEx( panel, FALSE );
	AddPanelToStackEx( panel, parent, FALSE );
	{
		PPANEL child = panel->child;
		while( child )
		{
			PutDisplayAboveEx( child, panel, FALSE );
			child = child->elder;
		}
	}
	if( bUpdate )
	{
		BuildSpaceTree();
		DispatchDrawEvents( NULL, panel, FALSE );
	}
}

RENDER_PROC( void, PutDisplayAbove )( PPANEL panel, PPANEL parent )
{
	PutDisplayAboveEx( panel, parent, TRUE );
}

//---------------------------------------------------------------------------

RENDER_PROC( void, ShovePanelBack )( PPANEL panel )
{
   // shoves to just above the root panel...
   // always is above this.  and one should
   // NEVER shove the root panel back.
   if( panel && g.pRootPanel )
   {
      GrabPanelFromStackEx( panel, FALSE );
      panel->below = g.pRootPanel->below;
      panel->above = g.pRootPanel;
      g.pRootPanel->below = panel;
   }
	BuildSpaceTree();
	DispatchRedrawEvents( panel, FALSE );

}

//---------------------------------------------------------------------------

RENDER_PROC( void, MovePanelAbove )( PPANEL panel, PPANEL above )
{
   if( panel && above )
   {
      GrabPanelFromStackEx( panel, FALSE );
      if( ( panel->below = above->below ) )
         panel->below->above = panel;
      panel->above = above;
      above->below = panel;
   }
	BuildSpaceTree();
	DispatchRedrawEvents( panel, FALSE );
}

//---------------------------------------------------------------------------

void CPROC ClearBackground( PTRSZVAL unused, PPANEL panel )
{
   //lprintf( WIDE("Clearing background on root image...") );
	ClearDisplay( panel );
   //DrawSpaces2();
}

//---------------------------------------------------------------------------

RENDER_PROC( void, EndDisplay )( void )
{
#ifdef __LINUX__
   EnterCriticalSec( &csThreadMessage );
	while( 1 )
	{
		INDEX new_tail, currmsg = thread_tail;
		thread_messages[thread_tail].ID = THREAD_DO_EXIT;
		new_tail = thread_tail + 1;
		if( new_tail >= MAX_THREAD_MESSAGES )
			new_tail -= MAX_THREAD_MESSAGES;
		if( new_tail != thread_head )
		{
			thread_tail = new_tail;
			if( currmsg == thread_head )
			{
#ifdef __NO_SDL__
				SendServiceEvent( 0, g.dwMsgBase + MSG_ThreadEventPost, NULL, 0 );
#else
				SDL_UserEvent event;
				event.type = SDL_USEREVENT;
				event.code = 0;
				event.data1 = NULL;
				event.data2 = NULL;
				SDL_PushEvent( (SDL_Event*)&event );
#endif
			}
			break;
		}
		LeaveCriticalSec( &csThreadMessage );
		EnterCriticalSec( &csThreadMessage );
	}
   LeaveCriticalSec( &csThreadMessage );
#endif
   // and wait for the exit to happen.
	while( global_pThread )
		Relinquish();
#ifdef __RAW_FRAMEBUFFER__
	if( g.PhysicalSurface )
	{
		UnmakeImageFile( g.PhysicalSurface );
		g.PhysicalSurface = NULL;
	}
#endif
	if( g.RealSurface )
	{
		UnmakeImageFile( g.RealSurface );
		g.RealSurface = NULL;
	}
#ifdef INTERNAL_BUFFER
	if( g.SoftSurface )
	{
		UnmakeImageFile( g.SoftSurface );
		g.SoftSurface = NULL;
	}
#endif
	if( g.pSpaceRoot )
		DeleteSpaceTree( &g.pSpaceRoot );
	g.pRootPanel = NULL;
#ifdef __LINUX__
#ifndef __NO_SDL__
	g.surface = 0;
#else
   // close the frame buffer device please.
#endif
#else
   g.hVideo = NULL;
#endif

}

RENDER_PROC( int, InitDisplay )( void )
{
	PPANEL pPanel;
   if( !InitMemory() )
   {
      // this is harsh - but we MUST evaluate why this happened.
      exit(1);
      return 0;
   }
   // retained mode is going to be a problem.

   if( !g.flags.configured )
   {
#ifndef _MSC_VER // no assembly available - config lib unavailable
		PCONFIG_HANDLER pch = CreateConfigurationEvaluator();
#endif
      g.width = 800;
		g.height = 600;
#ifndef __NO_SDL__
		AddIdleProc( ProcessDisplayMessages, 1 );
#endif
#ifndef _MSC_VER // no assembly available - config lib unavailable
		AddConfigurationMethod( pch, WIDE("display size is %i by %i"), SetSize );
		AddConfigurationMethod( pch, WIDE("display offset is %i, %i"), SetOffset  );
		// system colors need to be read/set by controls not display...
		// once upon a time this was going to take over that function
		// but the level of redundancy was then astoundingly redundant. :)
		//AddConfigurationMethod( pch, WIDE("system color %w %c"), SetColor );
# ifdef INTERNAL_BUFFER
		AddConfigurationMethod( pch, WIDE("soft cursor image %p"), SetSoftCursorImage );
      AddConfigurationMethod( pch, WIDE("soft cursor image alpha %i"), SetSoftCursorAlpha );
		AddConfigurationMethod( pch, WIDE("soft cursor behave %w"), SetSoftCursorBehavior );
		AddConfigurationMethod( pch, WIDE("soft cursor hotspot %i, %i"), SetCursorHotspot );
		AddConfigurationMethod( pch, WIDE("touch screen %b"), SetIsTouchScreen );
#  ifdef __LINUX__
		AddConfigurationMethod( pch, WIDE("mouse driver %w"), SetMouseDriver );
		AddConfigurationMethod( pch, WIDE("mouse device %p"), SetMouseDevice );
#  endif
# endif
# ifdef __LINUX__
		if( !ProcessConfigurationFile( pch, WIDE("Display.Config"), (PTRSZVAL)"." ) )
			ProcessConfigurationFile( pch, WIDE("/etc/Display.Config"), (PTRSZVAL)"/etc" );

# else
		ProcessConfigurationFile( pch, WIDE("Display.Config"), NULL );
# endif
      DestroyConfigurationEvaluator( pch );
#endif
		if( g.flags.touch_display )
		{
			PCONFIG_HANDLER pch = CreateConfigurationEvaluator();
			const char *driver = getenv( WIDE("SDL_MOUSEDRV") );
			char driverconfig[256];
         sprintf( driverconfig, WIDE("%s.Config"), driver );
			AddConfigurationMethod( pch, WIDE("elo min %i,%i"), SetEloMin );
			AddConfigurationMethod( pch, WIDE("elo max %i,%i"), SetEloMax );
			if( !ProcessConfigurationFile( pch, driverconfig, (PTRSZVAL)"." ) )
			{
				sprintf( driverconfig, WIDE("/etc/%s.Config"), driver );
				ProcessConfigurationFile( pch, driverconfig, (PTRSZVAL)"/etc" );
			}
		}
		g.flags.configured = 1;
	}
	if( !g.pRootPanel )
	{
		pPanel = (PPANEL)Allocate( sizeof( PANEL ) );
      MemSet( pPanel, 0, sizeof( PANEL ) );
      pPanel->KeyDefs = CreateKeyBinder();
      pPanel->common.x = 0;
      pPanel->common.y = 0;
      pPanel->common.width = g.width;
      pPanel->common.height = g.height;
      pPanel->common.background.Color = Color( 0, 0, 0 );
      pPanel->common.flags.backdrop_color = TRUE;
      //pPanel->name = AddName( WIDE("$Root Panel") );
      g.pTopMost = pPanel;
		g.pRootPanel = pPanel;
      SetRedrawHandler( pPanel, ClearBackground, 0 );
      SetFocusedPanel( pPanel );
   }
	else
	{
		return (g.pRootPanel != NULL);
	}
	g.pSpaceRoot = NULL; // kept in volatile ram. Recreated.

	{
      //void cleanup( void );
		//atexit( cleanup );
	}
	//GetMemStats( NULL, NULL, NULL, NULL );
	Log2( WIDE("Going to init: %d by %d  32"), g.width, g.height );
#ifndef WIN32
#if defined( __RAW_FRAMEBUFFER__ )
	{
		struct fb_var_screeninfo var;
      POINTER buffer;
		_32 size;
		int fbdev;
		fbdev = open("/dev/fb0", O_WRONLY);
		if( fbdev == -1 )
		{
			DebugBreak();
			xlprintf(LOG_ALWAYS)( "failed to open device : %s", strerror(errno) );
		}
		else
		{
		/* setup var structure */
			ioctl(fbdev, FBIOGET_VSCREENINFO, &var);

			var.xres = g.width;
			var.yres = g.height;
			var.xres_virtual = g.width;
			var.yres_virtual = g.height;

			var.activate = FB_ACTIVATE_ALL;
			var.nonstd = 0;
			var.grayscale = 0;
#ifdef __ARM__
			var.bits_per_pixel = 16;
#else
			var.bits_per_pixel = 32;
#endif
			var.accel_flags = 0;
			ioctl(fbdev, FBIOPUT_VSCREENINFO, &var);
			close( fbdev );

			size = g.width*g.height * (var.bits_per_pixel/8);
			buffer = OpenSpace( NULL, "/dev/fb0", &size );
			if( buffer )
			{
				lprintf( "Size is %d %dx%d=%d", size, g.width, g.height, (g.width)*(g.height)*sizeof(CDATA));
				g.PhysicalSurface = BuildImageFile( (PCOLOR)buffer, g.width, g.height );
				g.RealSurface = MakeImageFile( g.width, g.height );
			}
			else
			{
				xlprintf(LOG_ALWAYS)( "Failed to map?" );
			}
		}
	}
#endif
#if defined( __SDL_DYNAMIC__ )
// dynamic SDL will attempt to load libsdl.so
								  // will also load appropriate functions for those functions
	// referenced within this library.
#endif
#if defined( __SDL__ ) || defined( __SDL_DYNAMIC__ )
	// although SDL DOES build (easily) for the unit
	// at least what simple functionality is requried...
	// let's not, and say we did.
	if( SDL_Init(SDL_INIT_VIDEO ) )
	{
		char msg[256];
		sprintf( msg, WIDE("Fatal error: Failed to initialize SDL: %s"), SDL_GetError() );
		SystemLog( msg );
		Release( pPanel );
		g.pTopMost = g.pRootPanel = NULL;
		return 0;
	}
	// little quicker than normal repeat
	// 1/5 of a second and 40 chars per second...
	//atexit( MySDL_Quit );
	//the above atexit is handled by other things...

	SDL_EnableKeyRepeat( 200, 1000 / 40 );

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	{
		SDL_VideoInfo const *info;
		info = SDL_GetVideoInfo();
		lprintf( WIDE("Result info is: %d"), info->vfmt->BitsPerPixel );
	}
	g.surface = SDL_SetVideoMode( g.width, g.height
										 , 32 // this might be 16 if it were ARM
										, SDL_HWSURFACE
										  | SDL_NOFRAME
#ifndef _DEBUG
											| SDL_FULLSCREEN
#endif
//										  | SDL_OPENGL
										 );
	if( !g.surface )
	{
		Log1( WIDE("Fatal error: Failed to set mode: %s"), SDL_GetError() );
		DeinitMemory();
		return 0;
	}

	if( g.flags.touch_display )
  		SDL_ShowCursor( SDL_DISABLE );

	// cannot use this to actually draw to... but it has
	// all information of a normal image...
	if( SDL_MUSTLOCK( g.surface ) )
	{
		xlprintf( LOG_ALWAYS )( WIDE("Think that this will not work well - must lock surface.") );
		// continueing below to assume we own the buffer.
	}
	g.RealSurface = BuildImageFile( (PCOLOR)g.surface->pixels, g.width, g.height );
#endif

#else // this is else of ifndef WINDOWS - thereof ifDEF windows
	if( g.hVideo )
	{
		//Release( (POINTER)GetDisplayImage( g.hVideo ) );
		//Release( (POINTER)g.hVideo );
	}
#ifndef _DEBUG
	// SetVideoSize( g.width, g.height );
#endif
	if( !(g.hVideo = (PVIDEO)g.RenderInterface->_OpenDisplaySizedAt( 0, g.width, g.height, g.ofs_x, g.ofs_y )) )
	{
		Log2( WIDE("What we have here is failure to open display %d by %d")
				, g.width, g.height );
		DeinitMemory();
		return 0;
	}
	g.RenderInterface->_UpdateDisplayEx( (PPANEL)g.hVideo DBG_SRC );
	g.RenderInterface->_SetMouseHandler( (PPANEL)g.hVideo, Mouse, 0 );
	g.RenderInterface->_SetKeyboardHandler( (PPANEL)g.hVideo, KeyProcHandler, 0 );
	g.RenderInterface->_SetLoseFocusHandler( (PPANEL)g.hVideo, LoseFocusHandler, 0 );
	g.RealSurface = (ImageFile*)g.RenderInterface->_GetDisplayImage( (PPANEL)g.hVideo );
#endif // #ifndef WIN32

/* setup our internal soft buffer, if defined to use one...
 * a lot of times, the actual video surface is already
 * double-buffered, and this is redunant...
 * but, simplifies rendering things like the mouse cursor and
 * sprites
 */
#ifdef INTERNAL_BUFFER
	lprintf( WIDE("Oh - this internal buffer...") );
	g.SoftSurface = MakeImageFile( g.width, g.height );
	ClearImageTo( g.SoftSurface, BASE_COLOR_BLACK );
	pPanel->common.RealImage = g.SoftSurface;
#else
	lprintf( WIDE("Oh - no internal buffer...") );
	pPanel->common.RealImage = g.RealSurface;
#endif
	pPanel->common.StableImage = pPanel->common.RealImage;

	SetImageAuxRect( pPanel->common.RealImage, (P_IMAGE_RECTANGLE)pPanel->common.RealImage );
	pPanel->common.RealImage->flags |= IF_FLAG_IS_PANEL; // is a panel for sure.

	/* this builds the inital, root display node... shouldn't be needed... */
	BuildSpaceTree();
	//Log( WIDE("Making display process thread...") );
	// AddIdleEx( ProcessMessageThread, 0 );
	// starts the thread, and adds the idle proc?
#ifndef __RAW_FRAMEBUFFER__
	ThreadTo( ProcessMessageThread, 0 );
#endif
	return (g.pRootPanel!=NULL);
}

//---------------------------------------------------------------------------

int CPROC DefaultMouse( PTRSZVAL psvPanel, S_32 x, S_32 y, _32 b )
{
	PPANEL panel = (PPANEL)psvPanel;
	static int _x, _y, _b;
#ifdef MOUSE_EVENT_DEBUG
	 Log3( WIDE("Default mouse method: %d %d %08x"), x, y, b );
#endif
	if( panel->flags.dragging )
	{
		if( b & MK_LBUTTON )
		{
			int dx = x - _x
			  , dy = y - _y;
			//Log6( WIDE("Drag frame (%d-%d=%d,%d-%d=%d).")
			//	 , x, _x, dx
			  // 	 , y, _y, dy );
			MoveDisplayRel( panel, dx, dy );
			return 1;
		  }
		panel->flags.dragging = FALSE;
	}

	if( !(_b & MK_LBUTTON ) ) // check first down on dialog to drag
	{
		if( b & MK_LBUTTON )
		{
			_x = x;
				_y = y;
			//Log2( WIDE("Setting drag frame at (%d,%d)"), x, y );
			panel->flags.dragging = TRUE;
		}
	}
	 _b = b;
	return 1;
}

//---------------------------------------------------------------------------

RENDER_PROC( void, GetMousePosition )( S_32 *x, S_32 *y )
{
   // pass a renderer.
	if( x ) *x = mouse_x;
	if( y ) *y = mouse_y;
}

//----------------------------------------------------------------------------

void GetMouseState(S_32 * x, S_32 * y, _32 *b)
{
	GetMousePosition( x, y );
	if( b )
		(*b) = g._last_buttons;
}

//---------------------------------------------------------------------------

RENDER_PROC( void, SetMousePosition )( PPANEL panel, S_32 x, S_32 y )
{
	if( panel )
      x += panel->common.x;
	if( panel )
		y += panel->common.y;
#ifdef __LINUX__
	{
		EnterCriticalSec( &csThreadMessage );
		while( 1 )
		{
			INDEX new_tail, currmsg = thread_tail;
			thread_messages[currmsg].ID = THREAD_DO_SETMOUSEPOS;
			thread_messages[currmsg].data.mousepos.x = x;
			thread_messages[currmsg].data.mousepos.y = y;
			new_tail = thread_tail + 1;
			if( new_tail >= MAX_THREAD_MESSAGES )
				new_tail -= MAX_THREAD_MESSAGES;
			if( new_tail != thread_head )
			{
				thread_tail = new_tail;
				if( currmsg == thread_head )
				{
#ifdef __NO_SDL__
					SendServiceEvent( 0, g.dwMsgBase + MSG_ThreadEventPost, NULL, 0 );
#else
					SDL_UserEvent event;
					event.type = SDL_USEREVENT;
					event.code = 0;
					event.data1 = NULL;
					event.data2 = NULL;
					SDL_PushEvent( (SDL_Event*)&event );
#endif
				}
				break;
			}
			LeaveCriticalSec( &csThreadMessage );
			EnterCriticalSec( &csThreadMessage );
		}
		LeaveCriticalSec( &csThreadMessage );
	}
#else
	SetCursorPos( x, y );
#endif
   // actually update the mouse position here...
}

//---------------------------------------------------------------------------

PPANEL CreatePanel( _32 attributes
						, S_32 x, S_32 y
						, _32 width, _32 height
						, PPANEL parent)
{
	PPANEL pPanel, pParent;
   if( !g.pRootPanel )
   	InitDisplay();
   //pParent = g.pRootPanel;
   //if( !pParent )
	//	pParent = BeginDisplay();
	if( parent &&
		 ( attributes & PANEL_ATTRIBUTE_INTERNAL ) )
		pParent = parent;
   else
		pParent = g.pRootPanel;

   pPanel = (PPANEL)Allocate( sizeof( PANEL ) );
   MemSet( pPanel, 0, sizeof( PANEL ) );
   pPanel->KeyDefs = CreateKeyBinder();
   //pPanel->name
	if( width == -1 )
      width = ( pParent->common.width * 4 ) / 5;
	if( height == -1 )
      height = ( pParent->common.height * 4 ) / 5;

   // coincidentally - attr-border is the correct mask

	if( x == -1 && y == -1 )
	{
		GetMousePosition( &x, &y );
		x -= width/2;
		y -= height/2;
		if( x + (S_32)width > pParent->common.width )
			x = pParent->common.width - width;
		if( y + (S_32)height > pParent->common.height )
			y = pParent->common.height - height;
		if( x < 0 )
			x = 0;
		if( y < 0 )
			y = 0;
	}
   // these REALLY should come directly from the subimage.
   // what language has the idea - this symbol IS this other thing.
   pPanel->common.x = x;
   pPanel->common.y = y;
   pPanel->common.width = width;
   pPanel->common.height = height;

   //pPanel->name = AddName( name );
	pPanel->common.RealImage = MakeSubImage( pParent?pParent->common.RealImage:NULL
														, x, y
														, width, height );
	ClearImageTo( pPanel->common.StableImage = MakeImageFile( width, height ), 0 );
	pPanel->common.StableImage->flags |= IF_FLAG_IS_PANEL;
   pPanel->common.StableImage->pParent = pPanel->common.RealImage; /* lie! */
   //pPanel->common.StableImage->x = x;
   //pPanel->common.StableImage->y = y;

	SetImageAuxRect( pPanel->common.RealImage
						, (P_IMAGE_RECTANGLE)pPanel->common.RealImage );
	pPanel->common.RealImage->flags |= IF_FLAG_PANEL_ROOT | IF_FLAG_IS_PANEL;

	pPanel->common.parent = pParent;

   if( ( pPanel->parent = parent ) )
   {
      if( ( pPanel->elder = parent->child ) )
         pPanel->elder->younger = pPanel;
      parent->child = pPanel;
	}

	if( attributes & PANEL_ATTRIBUTE_INTERNAL )
		pPanel->flags.contained = 1;

   //pPanel->common.flags.invisible = 1;
   pPanel->common.flags.initial = 1; // don't immediate update on frame draw.
   pPanel->MouseMethod = DefaultMouse;
	pPanel->psvMouse = (PTRSZVAL)pPanel;
	pPanel->parent = parent;
   // initial creation of the display does not put it anywhere
	//if( parent )
	//	AddPanelToStack( pPanel, parent );
	//else
						//	AddPanelToStack( pPanel, g.pTopMost );
   // can't be focused if it's not shown.
						//SetFocusedPanel( pPanel );
   // and there's no redraw to dispatch if it's not visible yet.
	//DispatchRedrawEvents( pPanel, FALSE );
   //lprintf( "Resulting with panel %p", pPanel );
   return pPanel;
}

//---------------------------------------------------------------------------

RENDER_PROC( PPANEL, OpenDisplaySizedAt ) ( _32 attributes
                                    , _32 width, _32 height
                                    , S_32 x, S_32 y
                                    )
{
    return CreatePanel( attributes, x, y, width, height, NULL );
}

//---------------------------------------------------------------------------

RENDER_PROC( PPANEL, OpenDisplayAboveSizedAt ) ( _32 attributes
																, _32 width, _32 height
																, S_32 x, S_32 y
																, PPANEL parent
																)
{
    return CreatePanel( attributes, x, y, width, height, parent );
}

RENDER_PROC(PRENDERER, OpenDisplayAboveUnderSizedAt)(_32 Attributes, _32 width, _32 height, S_32 x,
                                                      S_32 y, PRENDERER above, PRENDERER below)
{
   return CreatePanel( Attributes, x, y, width, height, above );
}

//---------------------------------------------------------------------------

RENDER_PROC( void, ClearDisplayEx )( PPANEL pPanel DBG_PASS )
{
   //_xlprintf( 1 DBG_RELAY )( WIDE("clear display %p... uhh... root %p?"), pPanel, g.pRootPanel );
	DisplayBlatColor(pPanel->common.StableImage
						 ,0 ,0
						 ,(pPanel->common.StableImage)->real_width
						 ,(pPanel->common.StableImage)->real_height
						 , pPanel->common.background.Color );
	//if( !g.flags.in_mouse )
	// if it is in mouse, then this is scheduled only...
	// when this gets down and dirty, perhaps we should walk the space tree
	// and get only what's been updated to draw... clear root panel
	// will result in an entire screen redraw, when it may not
   // have been all that much dirtied...
	//UpdateDisplay( pPanel );
}
#undef ClearDisplay
RENDER_PROC( void, ClearDisplay )( PPANEL pPanel )
{
   ClearDisplayEx( pPanel DBG_SRC );
}

RENDER_PROC( void, CloseDisplay )( PPANEL pPanel )
{
   LOGICAL bPriorFocused;
	if( !pPanel ) // probably safe to assume it's already closed.
		return;
	//lprintf( WIDE("Closing display %p"), pPanel );
	if( pPanel->used )
	{
		//lprintf( WIDE("Marking panel as destroyed, delay destroy: %p") , pPanel );
		pPanel->flags.destroy = 1;
      return;
	}
   // this disallows recursive killing of the same dispaly.
	AddUse( pPanel );
   //Log1( WIDE("Closing a display %p"), pPanel );
	if( g.pMouseOwner == pPanel )
	{
		//Log( WIDE("Releaseing mouse ownership. ") );
		g.pMouseOwner = NULL;
      g.flags.auto_owned_mouse = 0;
		if( pPanel->flags.owning_parents_mouse )
		{
			//Log( WIDE("Reverting mouse ownership to parent...") );
         //Log( WIDE("But - if the parent is gone... then we didn't get linked in..") );
			g.pMouseOwner = pPanel->parent;
		}
	}
	if( g.pLastFocusedPanel == pPanel )
	{
      bPriorFocused = 1;
		g.pLastFocusedPanel = NULL;
	}
	else
      bPriorFocused = 0;
	if( g.pRootPanel == pPanel )
      g.pRootPanel = NULL;
	// pPanel->common.parent - don't care about this...
   // this is merely an upwards relationship for drawing purposes...
	if( pPanel->parent )
	{
      //lprintf( WIDE("Panel had a parent - unlinking") );
		if( pPanel->parent->child == pPanel )
		{
         //lprintf( WIDE("Is first child, setting parent's child to my elder") );
			if( ( pPanel->parent->child = pPanel->elder ) )
				pPanel->elder->younger = NULL;
		}
		else
		{
         //lprintf( WIDE("Is not first child, Just unlink from list") );
			if( pPanel->elder )
				pPanel->elder->younger = pPanel->younger;
			if( pPanel->younger )
				pPanel->younger->elder = pPanel->elder;
		}
		if( pPanel->child )
		{
			PPANEL sub = pPanel->child;
         //lprintf( WIDE("Panel has children.") );
			while( sub )
			{
            sub->parent = NULL;  // disrelate at least.
            sub = sub->elder;
			}
         //Log( WIDE("Child junk...") );
		}
	}
	else
	{
		// if there was no parent - then this will have no elders or youngers...
      //   lprintf( WIDE("uhh panel has no panent!") );
		if( pPanel->elder || pPanel->younger )
		{
         Log( WIDE("Fatality - I have siblings with no parent!") );
		}

	}

	// close children automagically?
	// otherwise we'll have to hang then back in...
   if( pPanel->child )
		Log( WIDE("Would have killed child displays...") );
       // so now we have to figure out their relation?!
	//if( pPanel->child )
	//	CloseDisplay( pPanel->child );
	if( pPanel->CloseMethod )
	{
      lprintf( WIDE("!!!!!!!!!!!! CLOSE METHOD!!!!!!!!!!!!!") );
		pPanel->CloseMethod( pPanel->psvClose );
	}
	GrabPanelFromStackEx( pPanel, TRUE );
	if( g.pFocusedPanel == pPanel || bPriorFocused )
	{
#ifdef DEBUG_FOCUS_EVENTS
		lprintf( WIDE("Clear focused panel...") );
#endif
		g.pFocusedPanel = NULL; // don't tell this display it's losing focus.
		if( g.pTopMost != g.pRootPanel )
		{
			SetFocusedPanel( g.pTopMost );  // won't be this one...
		}
		//else
      //   lprintf( "Top panel is the root panel?" );
	}

	// dispatch (fortunatly) just uses the panel's rectangle
	// to determine which areas to draw.
	// GrabPanelFromStack should have built a new spactree..
   //Log( WIDE("Generating all redraw events") );
	DispatchRedrawEvents( pPanel, FALSE );

	DestroyKeyBinder( pPanel->KeyDefs );
   pPanel->KeyDefs = NULL;
	UnmakeImageFile( pPanel->common.RealImage );
   UnmakeImageFile( pPanel->common.StableImage );
	Release( pPanel );
}

//---------------------------------------------------------------------------

static void CloseAllDisplays( PPANEL pDisp )
{
	if( pDisp )
	{
		//PPANEL pChild, pNext;
      lprintf( WIDE("closing younger") );
		CloseAllDisplays( pDisp->younger );
      lprintf( WIDE("Closing child") );
		CloseAllDisplays( pDisp->child );
//      if( !pDisp->flags.destroying )
			CloseDisplay( pDisp );
	}
}

//---------------------------------------------------------------------------
ATEXIT( Cleanup )
{
	CloseAllDisplays( g.pRootPanel );
#ifdef __LINUX__
	// shutdown SDL properly by posting
   // event to SDL Event handler
	{
		EnterCriticalSec( &csThreadMessage );
		while( 1 )
		{
			INDEX new_tail, currmsg = thread_tail;
			thread_messages[currmsg].ID = THREAD_DO_EXIT;
			new_tail = thread_tail + 1;
			if( new_tail >= MAX_THREAD_MESSAGES )
				new_tail -= MAX_THREAD_MESSAGES;
			if( new_tail != thread_head )
			{
				thread_tail = new_tail;
				if( currmsg == thread_head )
				{
#ifdef __NO_SDL__
					SendServiceEvent( 0, g.dwMsgBase + MSG_ThreadEventPost, NULL, 0 );
#else
					SDL_UserEvent event;
					event.type = SDL_USEREVENT;
					event.code = 0;
					event.data1 = NULL;
					event.data2 = NULL;
					SDL_PushEvent( (SDL_Event*)&event );
#endif
				}
				break;
			}
			LeaveCriticalSec( &csThreadMessage );
			EnterCriticalSec( &csThreadMessage );
		}
		LeaveCriticalSec( &csThreadMessage );
	}
#endif
}


static int HandleUpdateInitial( PPANEL pPanel )
{
	if( pPanel->common.flags.initial )
	{
		// now would be an excellent time to
      // draw yourself.
#ifdef REDRAW_DEBUG
		lprintf( WIDE("Initial draw dispatch events.") );
#endif
		pPanel->flags.cleaning = 1;
      /*
		if( pPanel->parent )
		{
			PPANEL parent = pPanel->parent;
         pPanel->parent = NULL;
         AddPanelToStack( pPanel, parent );
		}
		else
		*/
		//if( g.pTopMost == g.pRootPanel )


		SetFocusedPanel( pPanel );
		AddPanelToStack( pPanel, g.pTopMost );
		// if we wait, then the draw can be caught for the flush on focus change
      // - and - adding the panel to the stack will mark it dirty.
		pPanel->common.flags.initial = 0;
      // after rebuilding the space tree a dispatch can be done.
		DispatchDrawEvents( NULL, pPanel, FALSE );
#ifdef gDISPLAY_SERVICE
      //lprintf( WIDE("service, initial draw dispatch needs to return now.") );
		// as a service... the update comes in after all events
      // are processed
#endif
      return TRUE;
	}
   return FALSE;
}

//---------------------------------------------------------------------------

RENDER_PROC( void, UpdateDisplayPortionEx )( PPANEL pPanel, S_32 x, S_32 y, _32 wd, _32 ht DBG_PASS )
{
   // have to convert this to signed to work right.
	//S_32 height = ht, width = wd;

	//g.update_rect.flags.bHasContent = 0;
   //g.update_rect.flags.bTmpRect = 1;
#ifdef REDRAW_DEBUG
	lprintf( WIDE("Asked to update a region...") );
   if( pPanel != g.pRootPanel )
		_xlprintf( LOG_ALWAYS DBG_RELAY )( WIDE("Update Panel Part (%") _32fs WIDE(",%") _32fs WIDE(")-(%") _32f WIDE(",%") _32f WIDE(")"), x, y, wd, ht );
#endif
	if( pPanel->flags.destroy )
	{
		return;
	}
	if( HandleUpdateInitial( pPanel ) )
	{
      lprintf( WIDE("Initial draw dispatched... nothign to show until events are synced.") );
		return;
	}

	if( g.flags.in_mouse )
	{
#ifdef DEBUG_DIRTY_RECT
		lprintf( WIDE("Mosue active... queuing update.") );
#endif
      g.flags.was_in_mouse = 1;
		return;
	}
	// since we're hooked into the drawing code
	// I know exactly what region was updated...
	// I suppose the MIGHT want just a portion of what they had updated...
	// but... we'll see...
	RenderDirtyRegionsEx( pPanel DBG_RELAY );
}

//---------------------------------------------------------------------------

RENDER_PROC( void, UpdateDisplayEx )( PPANEL pPanel DBG_PASS )
{
	if( pPanel )
	{
      if( !HandleUpdateInitial( pPanel ) )
			RenderDirtyRegionsEx( pPanel DBG_RELAY );
#ifdef __LINUX__
#ifndef __arm__
		if( g.pRootPanel->flags.bOpenGL )
			SDL_GL_SwapBuffers();
#endif
#endif
		UpdateDisplayPortionEx( pPanel, 0, 0, pPanel->common.width, pPanel->common.height DBG_RELAY);
	}
	else
      Log( WIDE("NULL Panel updated.") );
}

#undef UpdateDisplay
RENDER_PROC( void, UpdateDisplay )( PPANEL pPanel )
{
   UpdateDisplayEx( pPanel DBG_SRC );
}

//---------------------------------------------------------------------------
/*
RENDER_PROC( void, SetPanelBackdrop )( PPANEL panel, enum backdrop_types type, ... )
{
   switch( type )
   {
   case BACKDROP_COLOR_VAL:
      break;
   case BACKDROP_IMAGE_VAL:
      break;
   case BACKDROP_IMAGE_TILED_VAL:
      break;
   case BACKDROP_CUSTOM_VAL:
      break;
   }
}
*/
//---------------------------------------------------------------------------

RENDER_PROC( void, GetPanelSize )( PPANEL region, int *width, int *height )
{
	// grab the drawable size - minus the frame size.
   if( width ) *width = region->common.RealImage->real_width;
   if( height ) *height = region->common.RealImage->real_height;
}

//---------------------------------------------------------------------------

RENDER_PROC( void, GetDisplaySize )( _32 *width, _32 *height )
{
	if(!g.pRootPanel )
	{
      InitDisplay();
	}
	if( width ) *width = g.width;
	if( height ) *height = g.height;
}

//---------------------------------------------------------------------------

RENDER_PROC( void, SetDisplaySize )( _32 width, _32 height )
{
	g.width = (S_16)width;
	g.height = (S_16)height;
#ifdef __LINUX__
#else
	// windows SetDisplaySize()?
#endif
	// resize display here.
}

//---------------------------------------------------------------------------

void GetDisplayPosition( PPANEL panel, S_32 *x, S_32 *y, _32 *width, _32 *height )
{
	if( !panel )
		return;
	if( x )
		*x = panel->common.x;
	if( y )
		*y = panel->common.y;
	if( width )
		*width = panel->common.width;
	if( height )
      *height = panel->common.height;
}

//---------------------------------------------------------------------------

RENDER_PROC( void, SizeDisplay )( PPANEL panel, _32 width, _32 height )
{
	// collect events based on prior tree...
	// (and old size)
	PENDING_RECT rect;
	rect.flags.bHasContent = 0;
   rect.flags.bTmpRect = 1;
	DispatchDrawEvents( &rect, panel, TRUE );
	// and then we wait...

	panel->common.width = width;
	panel->common.height = height;
	// collect events based on prior tree...
   // (and new size)

   //Log( WIDE("Size display and then resize the image...") );
	ResizeImage( panel->common.RealImage, width, height );
	ResizeImage( panel->common.StableImage, width, height );
	SetImageAuxRect( panel->common.RealImage, (P_IMAGE_RECTANGLE)panel->common.RealImage );
	BuildSpaceTree();
	// collect events based on new tree...

	DispatchDrawEvents( &rect, panel, FALSE );
}

//---------------------------------------------------------------------------

RENDER_PROC( void, SizeDisplayRel )( PPANEL panel
												, S_32 width, S_32 height )
{
	PENDING_RECT rect;
	rect.flags.bHasContent = 0;
   rect.flags.bTmpRect = 1;
	DispatchDrawEvents( &rect, panel, TRUE );
	panel->common.width += width;
	panel->common.height += height;
   //Log( WIDE("Size display and then resize the image relative...") );
	ResizeImage( panel->common.RealImage
				  , panel->common.width, panel->common.height );
	ResizeImage( panel->common.StableImage
				  , panel->common.width, panel->common.height );
	SetImageAuxRect( panel->common.RealImage, (P_IMAGE_RECTANGLE)panel->common.RealImage );
	BuildSpaceTree();
   DispatchDrawEvents( &rect, panel, FALSE );
}

//---------------------------------------------------------------------------

RENDER_PROC( void, MoveSizeDisplay )( PPANEL panel
													 , S_32 x, S_32 y
													 , S_32 width, S_32 height )
{
	PENDING_RECT rect;
	rect.flags.bHasContent = 0;
	if( !panel->common.flags.initial )
	{
		rect.flags.bTmpRect = 1;
		DispatchDrawEvents( &rect, panel, TRUE );
	}

	panel->common.width = width;
	panel->common.height = height;
	panel->common.x = x;
	panel->common.y = y;
	Log( WIDE("Size display and then resize the image relative...") );
	MoveImage( panel->common.RealImage
				, x
				, y );
	ResizeImage( panel->common.RealImage
				  , width, height );
	ResizeImage( panel->common.StableImage
				  , width, height );
	SetImageAuxRect( panel->common.RealImage, (P_IMAGE_RECTANGLE)panel->common.RealImage );
	BuildSpaceTree();
	if( !panel->common.flags.initial )
	{
		if( rect.flags.bHasContent )
			DispatchDrawEvents( &rect, panel, FALSE );
	}
}

//---------------------------------------------------------------------------

RENDER_PROC( void, MoveSizeDisplayRel )( PPANEL panel
													 , S_32 delx, S_32 dely
													 , S_32 width, S_32 height )
{
	MoveSizeDisplay( panel
					  , panel->common.x + delx
					  , panel->common.y + dely
					  , panel->common.width + width
					  , panel->common.height + height );
}

//---------------------------------------------------------------------------

RENDER_PROC( void, MoveDisplay )( PPANEL panel, S_32 x, S_32 y )
{
	PENDING_RECT rect;
	rect.flags.bHasContent = 0;
	rect.flags.bTmpRect = 1;
	DispatchDrawEvents( &rect, panel, TRUE );
	panel->common.x = x;
	panel->common.y = y;
	MoveImage( panel->common.RealImage, x, y );
	SetImageAuxRect( panel->common.RealImage, (P_IMAGE_RECTANGLE)panel->common.RealImage );
	// results in parts that are clean...
	BuildSpaceTree();
	DispatchDrawEvents( &rect, panel, FALSE );
}

//---------------------------------------------------------------------------

RENDER_PROC( void, MoveDisplayRel )( PPANEL panel, S_32 dx, S_32 dy )
{
	MoveDisplay( panel, dx + panel->common.x, dy + panel->common.y );
}

//---------------------------------------------------------------------------

RENDER_PROC( void, GetDisplayTextEx )( PCOMMON_PANEL_REGION pc, char *buffer, int buflen, int bCString )
{
	strncpy( buffer, pc->caption, buflen );
	buffer[buflen-1] = 0; // make sure we nul terminate..	
	if( bCString ) // use C processing on escapes....
	{
		int n, ofs, escape = 0;
		ofs = 0;
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

RENDER_PROC( void, SetDisplayTextEx )( PCOMMON_PANEL_REGION pc, char *text, int bCString )
{
	if( pc->caption )
      // this is safe here to delete const...
		Release( (char*)pc->caption );
	if( text )
	{
		pc->caption = (char*)Allocate( strlen( text ) + 1 );
		strcpy( (char*)pc->caption, text );
	}
	else
		pc->caption = NULL;

	/*
	if( pc->CaptionChanged )
		pc->CaptionChanged( pc );
	if( pc->DrawThySelf )
		pc->DrawThySelf( pc );
	*/	
}


RENDER_PROC( PSPRITE_METHOD, EnableSpriteMethod )(PRENDERER render, void(CPROC*RenderSprites)(PTRSZVAL psv, PRENDERER renderer, S_32 x, S_32 y, _32 w, _32 h ), PTRSZVAL psv )
{
	// add a sprite callback to the image.
	// enable copy image, and restore image
	PSPRITE_METHOD psm = (PSPRITE_METHOD)Allocate( sizeof( *psm ) );
	psm->renderer = render;
	psm->original_surface = MakeImageFile( render->common.RealImage->width, render->common.RealImage->height );
	psm->saved_spots = CreateDataQueue( sizeof( struct saved_location ) );
   psm->RenderSprites = RenderSprites;
	//AddLink( &render->sprites, psm );
	AddLink( &g.sprites, psm );
   return psm; // the sprite should assign this...
}

// this is a magic routine, and should only be called by sprite itself
// and therefore this is handed to the image library via an export into image library
// this is done this way, because the image library MUST exist before this library
// therefore relying on the linker to handle this export is not possible.
static void CPROC SavePortion( PSPRITE_METHOD psm, _32 x, _32 y, _32 w, _32 h )
{
	struct saved_location location;
   location.x = x;
   location.y = y;
   location.w = w;
	location.h = h;
	EnqueData( &psm->saved_spots, &location );
	lprintf( WIDE("Save Portion %") _32f WIDE(",%") _32f WIDE(" %") _32f WIDE(",%") _32f WIDE(""), x, y, w, h );
   /*
	BlotImageSizedEx( psm->original_surface, psm->renderer->pImage
						 , x, y
						 , x, y
						 , w, h
						 , 0
						 , BLOT_COPY );
                   */
}

PRELOAD( InitSetSavePortion )
{
   SetSavePortion( SavePortion );
}


RENDER_NAMESPACE_END

//---------------------------------------------------------------------------
