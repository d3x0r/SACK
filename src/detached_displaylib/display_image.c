//#define DIRTY_RECT_DEBUG
//#define NO_LOGGING
//#include "displaystruc.h"

//#include <display.h>
//#include "image.h"
#include <stdhdrs.h>
#include <timers.h>
#include "global.h"
#include "spacetree.h"

// this is the wrapper interface for image - this will
// observe the regions/panels and clip the operation
// potentially queing significant changes to a list which 
// may at a later time be flushed.
// This will produce least significant change updates.

// this will also only write into the container's surface!
RENDER_NAMESPACE
//-------------------------------------------------------------------------

 Image DisplayMakeSubImageEx( Image pImage
														, S_32 x, S_32 y
														, _32 width, _32 height DBG_PASS )
{
	Image newImage;
	newImage = MakeSubImageEx( pImage, x, y, width, height DBG_RELAY );
   SetImageAuxRect( newImage, (P_IMAGE_RECTANGLE)newImage );
	//Log7( WIDE("made a subimage: %p (%d,%d)-(%d,%d) %d x %d")
	//	 , newImage, x, y, x + width, y + height, width, height );
	if( pImage && ( pImage->flags & IF_FLAG_IS_PANEL ) )
	{
      //Log( WIDE("Cloning PANEL flag...") );
		newImage->flags |= IF_FLAG_IS_PANEL;
	}
	return newImage;
}

//-------------------------------------------------------------------------

// results in the panel-root that contains this image
// the rectangle passed (if any) contains the actual rectangle
// of this image within the parent image.
// any operations need to have this rectangle's x and y added 
// to them. 

Image FindImageRoot( Image _this, P_IMAGE_RECTANGLE realrect )
{
	if( realrect )
	{
		realrect->x = 0;
		realrect->y = 0;
      //Log2( WIDE("This image's dims (%d,%d)"), _this->real_width, _this->real_height );
		realrect->width = _this->real_width;
		realrect->height = _this->real_height;
	}
	if( !(_this->flags & IF_FLAG_IS_PANEL) )
	{
		return _this; // can use _this image natural.
	}
	while( _this && 
	       !( _this->flags & IF_FLAG_PANEL_ROOT ) && 
	       _this->pParent )
   {
		if( realrect )
		{
         //Log2( WIDE("Adding offset (%d,%d) to realrect base"), _this->real_x, _this->real_y );
			realrect->x += _this->real_x;
			realrect->y += _this->real_y;
		}
		if( ( _this = _this->pParent ) && realrect && realrect->width )
		{
			if( realrect->x > _this->real_width ||
			    realrect->y > _this->real_height ||
	   		 (realrect->x + (S_32)realrect->width) < 0 ||
		   	 (realrect->y + (S_32)realrect->height) < 0 )
			{
				//Log4( WIDE("Failing find! (%d>%d) or (%d>%d)")
				//	 , realrect->x, _this->real_width
				//	 , realrect->y, _this->real_height );
				realrect->width = 0;
			   realrect->height = 0;
			   return NULL;
			}
		}
	}
	if( realrect )
	{
	// add the real parent's position to the rectangle
	// _this will return absolute managed coordinates from 
	// upper left of real display surface.
		realrect->x += _this->real_x;
		realrect->y += _this->real_y;
	}
	return _this;
}

//-------------------------------------------------------------------------

#if 0
LOGICAL IntersectRectangles( P_IMAGE_RECTANGLE result, P_IMAGE_RECTANGLE pdr1, P_IMAGE_RECTANGLE pdr2 )
{
	if( !result )
		return FALSE;
	if( !pdr1 )
	{
		if( pdr2 )
			*result = *pdr2;
		else
			return FALSE;
	}
	else if( !pdr2 )
	{
		if( pdr1 )
			*result = *pdr1;
	}
	else
	{
		if( ( pdr1->x > ( pdr2->x + pdr2->width ) ) 
		  ||( pdr1->y > ( pdr2->y + pdr2->height ) ) 
        ||( ( pdr1->x + pdr1->width ) < pdr2->x ) 
        ||( ( pdr1->y + pdr1->height ) < pdr2->y ) ) 
			return FALSE;
		*result = *pdr1;
		if( result->x < pdr2->x )
		{
			result->width -= pdr2->x - pdr1->x;
			result->x = pdr2->x;
		}
		if( result->y < pdr2->y )
		{
			result->height -= pdr2->y - pdr1->y;
			result->y = pdr2->y;
		}
		if( ( result->x + result->width ) > ( pdr2->x + pdr2->width ) )
			result->width = pdr2->x + pdr2->width - result->x;
		if( ( result->y + result->height ) > ( pdr2->y + pdr2->height ) )
			result->height = pdr2->y + pdr2->height - result->y;
	}
	//Log4( WIDE("Result of %d,%d,%d,%d"), pdr1->x, pdr1->y, pdr1->width, pdr1->height );
	//Log4( WIDE("And       %d,%d,%d,%d"), pdr2->x, pdr2->y, pdr2->width, pdr2->height );
	//Log4( WIDE("Is        %d,%d,%d,%d"), result->x, result->y, result->width, result->height );
	return TRUE;	
}
#endif
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//  R.I.P.  Here lies the stubbed code to the image library to include
// panel clipping.
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
//-------------------------------------------------------------------------

//#define while( LoopHeader( image ) )(image) while( LoopHeader( image ) )

//	      Log8( WIDE("Bound to be : (%d,%d) - (%d,%d)  in (%d,%d)-(%d,%d)")
//	          , noderect.x, noderect.y, noderect.x + noderect.width, noderect.y + noderect.height
//	          , realrect.x, realrect.y, realrect.x + realrect.width, realrect.y + realrect.height );

#define LOOP_TRAILER  	   ; } 	\
	LeaveCriticalSec( &g.csSpaceRoot ); \
	FixImagePosition( image );

#define LOOP_HEADER(i) { \
	int result; \
	void *finddata = NULL; \
	EnterCriticalSec( &g.csSpaceRoot );   \
	while( ( result = LoopHeader( i, &finddata ) ) ) \
	   if( result < 0 ){ continue; } else

#define MY_ALPHA 32
//__inline
static int LoopHeaderEx( Image image, void **finddata DBG_PASS )
#define LoopHeader(i,fd) LoopHeaderEx( i,fd DBG_SRC )
{
#define DBG_LOOP_RELAY DBG_SRC
//#define DBG_LOOP_RELAY DBG_RELAY
   static CRITICALSECTION cs;
	static PSPACENODE found;
	static Image root;
	//SPACEPOINT min, max;
	//static void *finddata;
	static IMAGE_RECTANGLE realrect;
	IMAGE_RECTANGLE noderect;
   EnterCriticalSec( &cs );
#ifdef DIRTY_RECT_DEBUG
	_xlprintf( 1 DBG_LOOP_RELAY )( WIDE("--------------- Iterating... %p"), found  );
#endif
	if( !found )
	{
      //Log1( DBG_FILELINEFMT "Nothing found - setting find..." DBG_LOOP_RELAY,0 );
		if( !(root = FindImageRoot( image, &realrect )) )
		{
			LeaveCriticalSec( &cs );
			return 0;
		}
		noderect.width = ( noderect.x = realrect.x ) + realrect.width - 1;
		noderect.height = ( noderect.y = realrect.y ) + realrect.height - 1;
												 //Log( WIDE("Building find list...") );
 
		if( !g.pSpaceRoot )
		{
         lprintf( WIDE("This should never really happen... but if it does... maybe we're gracefully exiting?") );
			//DebugBreak();
			LeaveCriticalSec( &cs );
			return 0;
		}
		found = FindRectInSpaceEx( g.pSpaceRoot
										 , (P_IMAGE_POINT)&noderect.x
										 , (P_IMAGE_POINT)&noderect.width
										 , finddata
										  DBG_LOOP_RELAY);
	}
	else
	{
		noderect.width = ( noderect.x = realrect.x ) + realrect.width - 1;
		noderect.height = ( noderect.y = realrect.y ) + realrect.height - 1;
		found = FindRectInSpaceEx( NULL
										 , (P_IMAGE_POINT)&noderect.x
										 , (P_IMAGE_POINT)&noderect.width
										 , finddata
										  DBG_LOOP_RELAY);
	}
	for( ; found;
		 ( noderect.width = ( ( noderect.x = realrect.x ) + realrect.width - 1 ) )
		 , ( noderect.height = ( ( noderect.y = realrect.y ) + realrect.height - 1 ) )
		 , found = FindRectInSpaceEx( NULL
										  , (P_IMAGE_POINT)&noderect.x, (P_IMAGE_POINT)&noderect.width
										  , finddata
											DBG_LOOP_RELAY)
		)
	{
		//lprintf( WIDE("Had at least one found... %p"), found );
		IMAGE_RECTANGLE rect, imagerect;
		PPANEL thispanel = (PPANEL)GetNodeData( found );
#ifdef DIRTY_RECT_DEBUG
		Log1( WIDE("Found; %p"), found );
#endif
		rect = noderect;
		noderect.width  -= noderect.x - 1;
		noderect.height -= noderect.y - 1;
		if( thispanel->common.RealImage == root )
		{
		// mark it as potentiallyl dirty...
		// and the node is within the area on panel which is dirty...
        // rect = noderect;
#ifdef DIRTY_RECT_DEBUG
			{
				SPACEPOINT min, max;
				min[0] = noderect.x;
				min[1] = noderect.y;
				max[0] = noderect.width + noderect.x;
				max[1] = noderect.height + noderect.y;
				//DrawSpace( NULL, min, max, SetAlpha( BASE_COLOR_CYAN, MY_ALPHA/2 ) );
			}
#endif
			GetImageAuxRect( root, &imagerect );
			if( thispanel->common.flags.dirty_rect_valid )
			{
				imagerect = thispanel->common.dirty;
				//noderect.width  -= noderect.x - 1;
				// height = max-(min-1) max+ -(min - 1)
				//noderect.height -= noderect.y - 1;
				//GetImageAuxRect( root, &imagerect );
							 //#if 0
#ifdef DIRTY_RECT_DEBUG
					{
						SPACEPOINT min, max;
						min[0] = thispanel->common.dirty.x;
						min[1] = thispanel->common.dirty.y;
						max[0] = thispanel->common.dirty.width + thispanel->common.dirty.x;
						max[1] = thispanel->common.dirty.height + thispanel->common.dirty.y;
                  //if( image == g.SoftSurface )
						//	DrawSpace( NULL, min, max, SetAlpha( BASE_COLOR_BLUE, MY_ALPHA ) );
					}
					{
						SPACEPOINT min, max;
						min[0] = noderect.x;
						min[1] = noderect.y;
						max[0] = noderect.width + noderect.x;
                  max[1] = noderect.height + noderect.y;
                  //DrawSpace( NULL, min, max, SetAlpha( BASE_COLOR_YELLOW, MY_ALPHA ) );
					}
#endif
//#endif
				if( !IntersectRectangle( &rect, &thispanel->common.dirty, &noderect ) )
				{
#ifdef DIRTY_RECT_DEBUG
					lprintf( WIDE("Node NOT intersects... (%d,%d)-(%d,%d)  (%d,%d)-(%d,%d)  = (%d,%d)-(%d,%d)")
                       , noderect.x
                       , noderect.y
                       , noderect.width
                       , noderect.height
                       , thispanel->common.dirty.x
                       , thispanel->common.dirty.y
                       , thispanel->common.dirty.width
							 , thispanel->common.dirty.height
                       , rect.x
                       , rect.y
                       , rect.width
                       , rect.height
							 );
					//lprintf( WIDE("Node is outside the box.  Ignoring.") );
					{
						SPACEPOINT min, max;
						min[0] = noderect.x;
						min[1] = noderect.y;
						max[0] = noderect.width + rect.x;
                  max[1] = noderect.height + rect.y;
                  //DrawSpace( NULL, min, max, SetAlpha( BASE_COLOR_GREEN, MY_ALPHA + 32 ) );
					}
#endif
               continue;
				}
				else
				{
//#if 0
#ifdef DIRTY_RECT_DEBUG
					{
						SPACEPOINT min, max;
						min[0] = noderect.x;
						min[1] = noderect.y;
						max[0] = noderect.width + noderect.x;
                  max[1] = noderect.height + noderect.y;
                  //DrawSpace( NULL, min, max, SetAlpha( BASE_COLOR_BLUE, MY_ALPHA ) );
					}
					{
						SPACEPOINT min, max;
						min[0] = thispanel->common.dirty.x;
						min[1] = thispanel->common.dirty.y;
						max[0] = thispanel->common.dirty.width + thispanel->common.dirty.x;
                  max[1] = thispanel->common.dirty.height + thispanel->common.dirty.y;
                  //DrawSpace( NULL, min, max, SetAlpha( BASE_COLOR_RED, MY_ALPHA ) );
					}
#endif
#ifdef DIRTY_RECT_DEBUG

					{
						SPACEPOINT min, max;
						min[0] = rect.x;
						min[1] = rect.y;
						max[0] = rect.width + rect.x;
                  max[1] = rect.height + rect.y;
                  //DrawSpace( NULL, min, max, SetAlpha( BASE_COLOR_GREEN, MY_ALPHA ) );
					}
#endif
//#endif
#ifdef DIRTY_RECT_DEBUG
								 //#if 0
					lprintf( WIDE("Node intersects... (%d,%d)-(%d,%d)  (%d,%d)-(%d,%d)  = (%d,%d)-(%d,%d)")
                       , noderect.x
                       , noderect.y
                       , noderect.width
                       , noderect.height
                       , thispanel->common.dirty.x
                       , thispanel->common.dirty.y
                       , thispanel->common.dirty.width
							 , thispanel->common.dirty.height
                       , rect.x
                       , rect.y
                       , rect.width
                       , rect.height
							 );
#endif
//#endif
				}
			}
#ifdef DIRTY_RECT_DEBUG
			else
			{
				//GetImageAuxRect( root, &imagerect );
			//GetImageAuxRect( root, &rect );
            rect = noderect;
				{
					SPACEPOINT min, max;
					min[0] = rect.x;
					min[1] = rect.y;
					max[0] = rect.width + rect.x;
					max[1] = rect.height + rect.y;
					DrawSpace( NULL, min, max, SetAlpha( BASE_COLOR_RED, MY_ALPHA ) );
				}
				lprintf( WIDE("No dirty rect found on thispanel->") );
 			}
#endif
#ifdef DIRTY_RECT_DEBUG
         lprintf( WIDE("Marking node dirty.") );
				{
					SPACEPOINT min, max;
					min[0] = rect.x;
					min[1] = rect.y;
					max[0] = rect.width + rect.x;
					max[1] = rect.height + rect.y;
					//DrawSpace( NULL, min, max, SetAlpha( BASE_COLOR_RED, MY_ALPHA * 2 ) );
				}
#endif
			//MarkNodeDirty( found, &rect );
         imagerect = rect;
         // imagerect is the dirty rect if it was diryt...
			if( IntersectRectangle( &rect, &noderect, &imagerect ) )
			{
#ifdef DIRTY_RECT_DEBUG
					{
						SPACEPOINT min, max;
						min[0] = rect.x;
						min[1] = rect.y;
						max[0] = rect.width + rect.x;
                  max[1] = rect.height + rect.y;
                  //DrawSpace( NULL, min, max, SetAlpha( BASE_COLOR_GREEN, MY_ALPHA ) );
					}				//lprintf( WIDE("result bound... (%d,%d)-(%d,%d)"), rect.x, rect.y, rect.width, rect.height );
#endif
				MarkNodeDirty( found, &rect );
            rect.x -= realrect.x;
            rect.y -= realrect.y;
				SetImageBound( image, &rect );
				//lprintf( DBG_FILELINEFMT "Resulting NOW %ld %ld %ld %ld" DBG_LOOP_RELAY
				//		, rect.x
				//		, rect.y
				//		, rect.width
				//		, rect.height
				//		 );
				// lprintf( WIDE("This is the key to drawing.  If you continue, no application drawing will be ddone.") );
            //continue;
				LeaveCriticalSec( &cs );
#ifdef DIRTY_RECT_DEBUG
				lprintf( "marked node dirty, allowing draw into region" );
#endif
				return 1;
			}
			else
			{
#ifdef DIRTY_RECT_DEBUG
				lprintf( WIDE("Intersection failed: %p (%d,%d) -(%d,%d)  (%d,%d)-(%d,%d)")
                    , image
						  , noderect.x, noderect.y, noderect.width, noderect.height
						 , imagerect.x, imagerect.y, imagerect.width, imagerect.height );
#endif
				LeaveCriticalSec( &cs );
            lprintf( "found region, but failing draw..." );
				return -1;
			}
		}
#ifdef DIRTY_RECT_DEBUG
		else
		{
         lprintf( WIDE("thispanel->common.RealImage %p != %p"), thispanel->common.RealImage, root );
		}
#endif
		//Log1( WIDE("Found; %p"), found );
	}
	LeaveCriticalSec( &cs );
	return 0;
}

void DisplayPlot ( Image image, S_32 x, S_32 y, CDATA c )
{
   IMAGE_RECTANGLE rect;
	SPACEPOINT p;
	PSPACENODE found;
	Image root;
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
      (*g.ImageInterface->_plot)( image, x, y, c );
      return;
	}
	root = FindImageRoot( image, &rect );
	p[0] = x + rect.x;
	p[1] = y + rect.y;
   // let's just patch this here for a moment...
  	//plot( image, x, y, c );
	if( ( found = FindPointInSpace( g.pSpaceRoot, p, NULL ) ) )
	{
		// first panel found, with this point in it.
		// any other panel we'd have to have allowed this point
		// to pass through - thereby calling the background function.
		PPANEL panel = (PPANEL)GetNodeData( found );
		if( panel->common.RealImage == root )
			(*g.ImageInterface->_plot)( image, x, y, c );
	}
}

void  CPROC (*pDisplayPlot)( Image image, S_32 x, S_32 y, CDATA c ) = DisplayPlot;

//-------------------------------------------------------------------------
 CDATA  DisplayGetPixel ( Image image, S_32 x, S_32 y )
{
   IMAGE_RECTANGLE rect;
	SPACEPOINT p;
	PSPACENODE found;
	Image root;
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
      return (*g.ImageInterface->_getpixel)( image, x, y );
	}
	root = FindImageRoot( image, &rect );
	p[0] = x + rect.x;
	p[1] = y + rect.y;
	while( ( found = FindPointInSpace( g.pSpaceRoot, p, NULL ) ) )
	{
		// first panel found, with this point in it.
		// any other panel we'd have to have allowed this point
		// to pass through - thereby calling the background function.
		PPANEL panel = (PPANEL)GetNodeData( found );
		if( panel->common.RealImage == root )
		{
			return (*g.ImageInterface->_getpixel)( image, x, y );
		}
	}
   return 0; // this could be considered an error - 
             // a REAL black pixel would have alpha >0 therefore not be 0.
}

CDATA CPROC  (*pDisplayGetPixel)( Image image, S_32 x, S_32 y ) = DisplayGetPixel;
//-------------------------------------------------------------------------

 void  DisplayPlotAlpha ( Image image, S_32 x, S_32 y, CDATA c )
{
	SPACEPOINT p;
	PSPACENODE found;
	Image root;
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		(*g.ImageInterface->_plotalpha)( image, x, y, c );
		return;
	}
	root = FindImageRoot( image, NULL );
	p[0] = x + root->x;
	p[1] = y + root->y;
	found = g.pSpaceRoot;
	while( ( found = FindPointInSpace( found, p, NULL ) ) )
	{
		// first panel found, with this point in it.
		// any other panel we'd have to have allowed this point
		// to pass through - thereby calling the background function.
		PPANEL panel = (PPANEL)GetNodeData( found );
		if( panel->common.RealImage == root )
		{
			(*g.ImageInterface->_plotalpha)( image, x, y, c );
			break;
		}
		break;
	}
}

void  CPROC (*pDisplayPlotAlpha)( Image image, S_32 x, S_32 y, CDATA c ) = DisplayPlotAlpha;

//-------------------------------------------------------------------------

 void  DisplayLine ( Image image, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		(*g.ImageInterface->_do_line)( image, x, y, xto, yto, color );
		return;
	}

	LOOP_HEADER( image )
			(*g.ImageInterface->_do_line)( image
						, x
						, y
						, xto
						, yto, color );
	LOOP_TRAILER
}

void  CPROC (*pDisplayLine)( Image image, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA c ) = DisplayLine;
//-------------------------------------------------------------------------

 void  DisplayLineV ( Image image, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color, void(*proc)(Image Image, S_32 x, S_32 y, _32 d ) )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		//do_lineExV( image, x, y, xto, yto, color, proc );
		return;
	}

	LOOP_HEADER( image )
		//do_lineExV( image, x, y
		//			 , xto, yto, color, proc );
	LOOP_TRAILER
}

void  CPROC (*pDisplayLineV)( Image image, S_32 x, S_32 y
										, S_32 xto, S_32 yto
										, CDATA c
										, void(*proc)(Image Image, S_32 x, S_32 y, _32 d ) ) = DisplayLineV;
//-------------------------------------------------------------------------

 void  DisplayLineAlpha ( Image image, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA color )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
      do_lineAlpha( image, x, y, xto, yto, color );
      return;
	}

	LOOP_HEADER( image )
		do_lineAlpha( image, x, y, xto, yto, color );
	LOOP_TRAILER
}

void  CPROC (*pDisplayLineAlpha)( Image image, S_32 x, S_32 y, S_32 xto, S_32 yto, CDATA c ) = DisplayLineAlpha;
//-------------------------------------------------------------------------

 void  DisplayHLine ( Image image, S_32 y, S_32 xfrom, S_32 xto, CDATA color )
{
	if( !image )
		return;
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
      //Log4( WIDE("Do hline on bare %p at %d (%d-%d)"), image,  y, xfrom, xto );
      (*g.ImageInterface->_do_hline)( image, y, xfrom, xto, color );
      return;
	}

	LOOP_HEADER( image )
	{
      //Log4( WIDE("Do hline on %p at %d (%d-%d)"), image,  y, xfrom, xto );
		(*g.ImageInterface->_do_hline)( image, y, xfrom, xto, color );
	}
	LOOP_TRAILER
}

void  CPROC (*pDisplayHLine)( Image image, S_32 x, S_32 y, S_32 yto, CDATA c ) = DisplayHLine;
//-------------------------------------------------------------------------

 void  DisplayHLineAlpha ( Image image, S_32 y, S_32 xfrom, S_32 xto, CDATA color )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
      (*g.ImageInterface->_do_hlineAlpha)( image, y, xfrom, xto, color );
      return;
	}

	LOOP_HEADER( image )
		(*g.ImageInterface->_do_hlineAlpha)( image, y, xfrom, xto, color );
	LOOP_TRAILER
}

void  CPROC (*pDisplayHLineAlpha)( Image image, S_32 x, S_32 y, S_32 yto, CDATA c ) = DisplayHLineAlpha;
//-------------------------------------------------------------------------

 void  DisplayVLine ( Image image, S_32 x, S_32 yfrom, S_32 yto, CDATA color )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
      lprintf( WIDE("DisplayVLine Natural image - no bound check.") );
      (*g.ImageInterface->_do_vline)( image, x, yfrom, yto, color );
      return;
	}

		//lprintf( WIDE("DisplayVLine Panel image") );
	LOOP_HEADER( image )
	{
		//lprintf( WIDE("DisplayVLine Panel image - bound check(found).") );
		(*g.ImageInterface->_do_vline)( image, x, yfrom, yto, color );
	}
	LOOP_TRAILER
}

void  CPROC (*pDisplayVLine)( Image image, S_32 x, S_32 yfrom, S_32 yto, CDATA c ) = DisplayVLine;
//-------------------------------------------------------------------------

 void  DisplayVLineAlpha ( Image image, S_32 x, S_32 yfrom, S_32 yto, CDATA color )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
      (*g.ImageInterface->_do_vlineAlpha)( image, x, yfrom, yto, color );
      return;
	}

	LOOP_HEADER( image )
		(*g.ImageInterface->_do_vlineAlpha)( image, x, yfrom, yto, color );
	LOOP_TRAILER
}

void  CPROC (*pDisplayVLineAlpha)( Image image, S_32 x, S_32 y, S_32 yto, CDATA c ) = DisplayVLineAlpha;
//-------------------------------------------------------------------------


 void  DisplayBlatColor ( Image image, S_32 x, S_32 y, _32 w, _32 h, CDATA color )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
      //lprintf( WIDE("natural image.") );
      BlatColor( image, x, y, w, h, color );
      return;
	}
   //lprintf( WIDE("blatcolor...") );
	LOOP_HEADER( image )
	{
      //lprintf( WIDE("Uhmm blatsection %d,%d %d,%d"), x, y, w, h );
		BlatColor( image, x, y, w, h, color );
	}
	LOOP_TRAILER
}

 void  DisplayBlatColorAlpha ( Image image, S_32 x, S_32 y, _32 w, _32 h, CDATA color )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
      BlatColorAlpha( image, x, y, w, h, color );
      return;
	}

	LOOP_HEADER( image )
		BlatColorAlpha( image, x, y, w, h, color );
	LOOP_TRAILER
}

 void  DisplayBlotImageEx ( Image dest
						, Image image
						, S_32 xd, S_32 yd
						, _32 transparency
						, _32 method
						, ... )
{
	CDATA r,_g,b;
	va_list colors;
	va_start( colors, method );
	r = va_arg( colors, CDATA );
	_g = va_arg( colors, CDATA );
	b = va_arg( colors, CDATA );
	if( !( dest->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		BlotImageEx( dest, image, xd, yd, transparency, method
					  ,r,_g,b
					  );
      return;
	}

	LOOP_HEADER( dest )
		BlotImageEx( dest, image
					  , xd, yd
					  , transparency
					  , method
					  , r,_g,b
					  );
	LOOP_TRAILER
}

//-------------------------------------------------------------------------

 void  DisplayBlotImageSizedEx ( Image dest
						, Image image
						, S_32 xd, S_32 yd
						, S_32 xs, S_32 ys, _32 ws, _32 hs
						, _32 transparency
						, _32 method
						, ... )
{
	CDATA r,_g,b;
	va_list colors;
	va_start( colors, method );
	r = va_arg( colors, CDATA );
	_g = va_arg( colors, CDATA );
	b = va_arg( colors, CDATA );
					  
	if( !( dest->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		BlotImageSizedEx( dest, image, xd, yd, xs, ys, ws, hs, transparency, method
							 , r,_g,b
							 );
		return;
	}
   //Log( WIDE("Doing update in portions...") );
	LOOP_HEADER( dest )
	{
		BlotImageSizedEx( dest, image
							 , xd, yd
							 , xs, ys
							 , ws, hs
							 , transparency
							 , method
							 , r,_g,b
							 );
	}
	LOOP_TRAILER
}

//-------------------------------------------------------------------------

 void  DisplayBlotScaledImageSizedEx ( Image dest
						, Image image
						, S_32 xd, S_32 yd, _32 wd, _32 hd
						, S_32 xs, S_32 ys, _32 ws, _32 hs
						, _32 transparency
						, _32 method
						, ... )
{
	CDATA r,_g,b;
	va_list colors;
	va_start( colors, method );
	r = va_arg( colors, CDATA );
	_g = va_arg( colors, CDATA );
	b = va_arg( colors, CDATA );
	//lprintf( WIDE("%p has panel flag: %d"), dest, dest->flags & IF_FLAG_IS_PANEL );
	if( !( dest->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		//lprintf( WIDE("(bare)BlotScaledImageSized %p to %p  (%d,%d)-(%d,%d) too (%d,%d)-(%d,%d)")
		//	  , image, dest, xs, ys, xs+ws, ys+hs, xd, yd, xd+wd, yd+hd );

		BlotScaledImageSizedEx( dest, image
									 , xd, yd
									 , wd, hd
									 , xs, ys
									 , ws, hs, transparency, method
									 ,r,_g,b
									 );
      return;
	}
	//lprintf( WIDE("Therefore try to do this in sections...") );
   LOOP_HEADER( dest )
	{
		//lprintf( WIDE("(panl)BlotScaledImageSized %p to %p  (%d,%d)-(%d,%d) too (%d,%d)-(%d,%d)")
		//		 , image, dest, xs, ys, xs+ws, ys+hs, xd, yd, xd+wd, yd+hd );
		BlotScaledImageSizedEx( dest, image
									 , xd, yd
									 , wd, hd
									 , xs, ys
									 , ws, hs
									 , transparency, method
                            ,r,_g,b
									  );
	}
	LOOP_TRAILER
}

//-------------------------------------------------------------------------

 void  DisplayPutCharacterFont			( Image image, S_32 x, S_32 y, CDATA fore, CDATA back, TEXTCHAR c, SFTFont font )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		PutCharacterFont( image, x, y, fore, back, c, font );
	}
	else
	{
		LOOP_HEADER( image )
			PutCharacterFont( image, x, y, fore, back, c, font );
		LOOP_TRAILER
	}
	return;
}

//-------------------------------------------------------------------------

 void  DisplayPutCharacterVerticalFont	( Image image, S_32 x, S_32 y, CDATA fore, CDATA back, TEXTCHAR c, SFTFont font )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		PutCharacterVerticalFont( image, x, y, fore, back, c, font );
	}
	else
	{
		LOOP_HEADER( image )
			PutCharacterVerticalFont( image, x, y, fore, back, c, font );
		LOOP_TRAILER;
	}
	return; // should return string length along character span
}

//-------------------------------------------------------------------------

 void  DisplayPutCharacterInvertFont			( Image image, S_32 x, S_32 y, CDATA fore, CDATA back, TEXTCHAR c, SFTFont font )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		PutCharacterInvertFont( image, x, y, fore, back, c, font );
	}
	else
	{
		LOOP_HEADER( image )
			PutCharacterInvertFont( image, x, y, fore, back, c, font );
		LOOP_TRAILER;
	}
	return; // should return string length along character span
}

//-------------------------------------------------------------------------

 void  DisplayPutCharacterVerticalInvertFont	( Image image, S_32 x, S_32 y, CDATA fore, CDATA back, TEXTCHAR c, SFTFont font )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		PutCharacterVerticalInvertFont( image, x, y, fore, back, c, font );
	}
	else
	{
		LOOP_HEADER( image )
			PutCharacterVerticalInvertFont( image, x, y, fore, back, c, font );
		LOOP_TRAILER;
	}

	return; // should return string length along character span
}

//-------------------------------------------------------------------------

 void  DisplayPutStringFontEx				( Image image, S_32 x, S_32 y, CDATA fore, CDATA back, CTEXTSTR pc, _32 nLen, SFTFont font )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		PutStringFontEx( image, x, y, fore, back, pc, nLen, font );
	}
	else
	{
		LOOP_HEADER( image )
			PutStringFontEx( image, x, y, fore, back, pc, nLen, font );
		LOOP_TRAILER;
	}
	return; // should return string length along character span
}

//-------------------------------------------------------------------------

 void  DisplayPutStringVerticalFontEx	( Image image, S_32 x, S_32 y, CDATA fore, CDATA back, CTEXTSTR pc, _32 nLen, SFTFont font )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		PutStringVerticalFontEx( image, x, y, fore, back, pc, nLen, font );
	}
	else
	{
		LOOP_HEADER( image )
			PutStringVerticalFontEx( image, x, y, fore, back, pc, nLen, font );
		LOOP_TRAILER;
	}
	return; // should return string length along character span
}

//-------------------------------------------------------------------------

 void  DisplayPutStringInvertFontEx				( Image image, S_32 x, S_32 y, CDATA fore, CDATA back, CTEXTSTR pc, _32 nLen, SFTFont font )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		PutStringInvertFontEx( image, x, y, fore, back, pc, nLen, font );
	}
	else
	{
		LOOP_HEADER( image )
			PutStringInvertFontEx( image, x, y, fore, back, pc, nLen, font );
		LOOP_TRAILER;
	}
	return; // should return string length along character span
}

//-------------------------------------------------------------------------

 void  DisplayPutStringInvertVerticalFontEx	( Image image, S_32 x, S_32 y, CDATA fore, CDATA back, CTEXTSTR pc, _32 nLen, SFTFont font )
{
	if( !( image->flags & IF_FLAG_IS_PANEL ) )
	{ // natural images can use the natural functions
		//return
			PutStringInvertVerticalFontEx( image, x, y, fore, back, pc, nLen, font );
	}
	else
	{
		LOOP_HEADER( image )
			PutStringInvertVerticalFontEx( image, x, y, fore, back, pc, nLen, font );
		LOOP_TRAILER;
	}
	return; // should return string length along character span
}


//-------------------------------------------------------------------------

 void  DisplaySetImageBound ( Image pImage, P_IMAGE_RECTANGLE bound )
{
   //lprintf( WIDE("Setting boundary (aux image %p rect)=(%d,%d)-(%d,%d)"), pImage, bound->x, bound->y, bound->width, bound->height );
   SetImageAuxRect( pImage, bound );
   SetImageBound( pImage, bound );
}

//-------------------------------------------------------------------------

 void  DisplayFixImagePosition ( Image pImage )
{
	// Passing the first part of the image as a rectangle
   // will be the true, maximum image rectangle....
	SetImageAuxRect( pImage, (P_IMAGE_RECTANGLE)pImage );
   FixImagePosition( pImage );
}

//-------------------------------------------------------------------------

static void SetImageFlag( Image image )
{
	while( image )
	{
      image->flags |= IF_FLAG_IS_PANEL;
		SetImageFlag( image->pChild );
      image = image->pElder;
	}
}

//-------------------------------------------------------------------------

static void ClearImageFlag( Image image )
{
	while( image )
	{
      image->flags &= ~IF_FLAG_IS_PANEL;
		SetImageFlag( image->pChild );
      image = image->pElder;
	}
}

//-------------------------------------------------------------------------

 void  DisplayAdoptSubImage ( Image pFoster, Image pOrphan )
{
	AdoptSubImage( pFoster, pOrphan );
	if( pFoster->flags & IF_FLAG_IS_PANEL )
      SetImageFlag( pOrphan );
}

//-------------------------------------------------------------------------

 void  DisplayOrphanSubImage ( Image pImage )
{
	OrphanSubImage( pImage );
	if( pImage->flags & IF_FLAG_IS_PANEL )
      ClearImageFlag( pImage );
}


//-------------------------------------------------------------------------

void DoNothing( void )
{
}

//-------------------------------------------------------------------------

static IMAGE_INTERFACE DisplayImageInterface = {
    // these should all point directly
	// to the image library from whence they came.
	(void CPROC (*)(Image, _32))DoNothing
   , NULL //SetBlotMethod
	, NULL //BuildImageFileEx
	, NULL //MakeImageFileEx
	, DisplayMakeSubImageEx
   , NULL //RemakeImageEx
   , NULL //LoadImageFileEx
   , NULL //UnmakeImageFileEx
   , DisplaySetImageBound //SetImageBound
   , DisplayFixImagePosition
	, NULL //ResizeImageEx
	, NULL //MoveImage
	 // these are the drawing methods - subject to clipping
	 // by this library.
	, DisplayBlatColor
	, DisplayBlatColorAlpha                   
	, DisplayBlotImageEx                      
	, DisplayBlotImageSizedEx
   , DisplayBlotScaledImageSizedEx

	, &pDisplayPlot
   , &pDisplayPlotAlpha
   , &pDisplayGetPixel

	,&pDisplayLine      
	//,&pDisplayLineV
	,&pDisplayLineAlpha 
	,&pDisplayHLine     
	,&pDisplayVLine     
	,&pDisplayHLineAlpha
   ,&pDisplayVLineAlpha

   ,NULL //GetDefaultFont                           [5~
   ,NULL //GetFontHeight
   ,NULL //GetStringSizeFontEx

	,DisplayPutCharacterFont                 
	,DisplayPutCharacterVerticalFont         
	,DisplayPutCharacterInvertFont            
   ,DisplayPutCharacterVerticalInvertFont

	,DisplayPutStringFontEx                  
	,DisplayPutStringVerticalFontEx          
	,DisplayPutStringInvertFontEx            
   ,DisplayPutStringInvertVerticalFontEx
   /* these from here down are filled in by GetInterface */
   ,NULL //GetMaxStringLengthFont
   ,NULL //GetImageSize
   ,NULL
};

#undef DropImageInterface
#undef GetImageInterface

// otherwise getimageinterface below will be wrong...
static POINTER CPROC _DisplayGetImageInterface( void )
{
	InitDisplay();
   DisplayImageInterface._SetBlotMethod        = SetBlotMethod;
	DisplayImageInterface._BuildImageFileEx	  = BuildImageFileEx;
	DisplayImageInterface._MakeImageFileEx	     = MakeImageFileEx;
	//DisplayImageInterface._MakeSubImageEx	     = MakeSubImageEx;
   DisplayImageInterface._RemakeImageEx		  = RemakeImageEx;
   DisplayImageInterface._LoadImageFileEx	     = LoadImageFileEx;
   DisplayImageInterface._UnmakeImageFileEx    = UnmakeImageFileEx;
   DisplayImageInterface._SetImageBound		  = DisplaySetImageBound;
   DisplayImageInterface._FixImagePosition	  = DisplayFixImagePosition;
	DisplayImageInterface._ResizeImageEx		  = ResizeImageEx;
	DisplayImageInterface._MoveImage			     = MoveImage;

	DisplayImageInterface._GetDefaultFont        = GetDefaultFont;
   DisplayImageInterface._GetFontHeight			= GetFontHeight;
	DisplayImageInterface._GetStringSizeFontEx	= GetStringSizeFontEx;

	DisplayImageInterface._GetMaxStringLengthFont = GetMaxStringLengthFont;
	DisplayImageInterface._GetImageSize           = GetImageSize;

   DisplayImageInterface._LoadFont = LoadFont;
   DisplayImageInterface._UnloadFont = UnloadFont;
   /* these really have no meaning unless client/server is in place */
	DisplayImageInterface._BeginTransferData = NULL;
	DisplayImageInterface._ContinueTransferData = NULL;
	DisplayImageInterface._DecodeTransferredImage = NULL;
	DisplayImageInterface._DecodeMemoryToImage = g.ImageInterface->_DecodeMemoryToImage;
	DisplayImageInterface._AcceptTransferredFont = NULL;

	DisplayImageInterface._ColorAverage = g.ImageInterface->_ColorAverage;
   DisplayImageInterface._IntersectRectangle = IntersectRectangle;
	DisplayImageInterface._GetImageAuxRect = GetImageAuxRect;
	DisplayImageInterface._SetImageAuxRect = SetImageAuxRect;
	DisplayImageInterface._OrphanSubImage = DisplayOrphanSubImage;
	DisplayImageInterface._AdoptSubImage = DisplayAdoptSubImage;
	DisplayImageInterface._MergeRectangle = MergeRectangle;
   DisplayImageInterface._GetImageAuxRect = GetImageAuxRect;
   DisplayImageInterface._SetImageAuxRect = SetImageAuxRect;
	DisplayImageInterface._GetImageSurface = GetImageSurface;
   DisplayImageInterface._InternalRenderFont     = InternalRenderFont;
	DisplayImageInterface._InternalRenderFontFile = InternalRenderFontFile;
	DisplayImageInterface._RenderScaledFontData         = RenderScaledFontData;
   DisplayImageInterface._RenderFontFileEx       = RenderFontFileEx;
   DisplayImageInterface._DestroyFont            = DestroyFont;
	DisplayImageInterface._GetFontRenderData      = GetFontRenderData;
	DisplayImageInterface._SetFontRendererData      = SetFontRendererData;
   DisplayImageInterface._global_font_data         = g.ImageInterface->_global_font_data;
   DisplayImageInterface._GetGlobalFonts         = g.ImageInterface->_GetGlobalFonts;

	DisplayImageInterface._MakeSpriteImageFileEx = g.ImageInterface->_MakeSpriteImageFileEx;
	DisplayImageInterface._MakeSpriteImageEx = g.ImageInterface->_MakeSpriteImageEx;
	DisplayImageInterface._rotate_scaled_sprite = g.ImageInterface->_rotate_scaled_sprite;
	DisplayImageInterface._rotate_sprite = g.ImageInterface->_rotate_sprite;
	DisplayImageInterface._BlotSprite = g.ImageInterface->_BlotSprite;
	DisplayImageInterface._SetSpriteHotspot = g.ImageInterface->_SetSpriteHotspot;
	DisplayImageInterface._SetSpritePosition = g.ImageInterface->_SetSpritePosition;
	DisplayImageInterface._DecodeMemoryToImage = g.ImageInterface->_DecodeMemoryToImage;




	//InitMemory();


	return (POINTER)&DisplayImageInterface;
}

 PIMAGE_INTERFACE  GetImageInteface(void )
{
   return (PIMAGE_INTERFACE)_DisplayGetImageInterface();
}

static void CPROC _DisplayDropImageInterface( POINTER p )
{
}

#undef DropImageInterface
 void  DropImageInterface ( PIMAGE_INTERFACE p )
{
// do stuff here methinks...
   _DisplayDropImageInterface( p );
}

#ifndef DISPLAY_SERVICE
PRELOAD( DisplayImageRegisterInterface )
{
   lprintf( WIDE("Registering display image interface") );
   RegisterInterface( "display_image", _DisplayGetImageInterface, _DisplayDropImageInterface );
}
#endif

void DoSetImagePanelFlag( Image first, Image parent, Image image )
{
	//lprintf( WIDE("Setting panel image flag on %p under %p (%d)")
	//		 , image, parent, parent->flags&IF_FLAG_IS_PANEL );
	if( parent->flags & IF_FLAG_IS_PANEL )
	{
		while( image )
		{
			//lprintf( WIDE("Setting panel image flag on %p under %p"), image, parent );
			image->flags |= IF_FLAG_IS_PANEL;
			if( image->pChild )
				DoSetImagePanelFlag( first, image, image->pChild );
         if( image != first )
				image = image->pElder;
			else
            break;
		}
	}
}

void DoClearImagePanelFlag( Image first, Image image )
{
	while( image )
	{
      //lprintf( WIDE("clearing panel image flag on %p"), image );
		image->flags &= ~IF_FLAG_IS_PANEL;
		if( image->pChild )
			DoClearImagePanelFlag( first, image->pChild );
      if( first != image )
			image = image->pElder;
		else
			break;
	}
}

void SetImagePanelFlag( Image parent, Image image )
{
   DoSetImagePanelFlag( image, parent, image );
}

void ClearImagePanelFlag( Image image )
{
   DoClearImagePanelFlag( image, image );
}

RENDER_NAMESPACE_END

