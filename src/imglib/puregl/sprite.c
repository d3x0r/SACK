/*
 *   Crafted by James Buckeyne
 *     there may remain some commented out code that existed
 *     once upon a time within the allegro library.  This is an
 *     entirely new implmeentation, however, which does not use
 *     trig functions such as arctan, tan, cotan, (or whatever it was)
 *
 *   2006 Freedom Collective
 *
 */

#ifndef IMAGE_LIBRARY_SOURCE
#define IMAGE_LIBRARY_SOURCE
#endif

// pragma warning 367
#include <stdhdrs.h>
#include <stdio.h>
#include <string.h>
#define LIBRARY_DEF
#include <sharemem.h>
#include <vectlib.h>
#include <imglib/imagestruct.h>
#define NEED_ALPHA2
#include "blotproto.h"
#include "image.h"
#include "../sprite_local.h"

#include "local.h"

//#define DEBUG_TIMING
#define OUTPUT_IMAGE

#define _farnspokeb(a,v) ( ( *(char*)(a) ) = (v) )
#define _farnspokew(a,v) ( ( *(short*)(a) ) = (v) )
#define _farnspokel(a,v) ( ( *(long*)(a) ) = (v) )

IMAGE_NAMESPACE

#define SET_POINTS(a,b,c,d,e,f) {    \
	if( a==d ) points[0].flags.samepoint=1; else points[0].flags.samepoint=0; \
   points[0].flags.no_left = points[0].flags.no_right = 0; \
	points[0].left.x = x##a;     \
	points[0].left.y= y##a;      \
   points[0].left.vertex_number = a-1; \
	points[0].right.x = x##d;      \
	points[0].right.y= y##d;      \
   points[0].right.vertex_number = d-1; \
	if( b==e ) points[1].flags.samepoint=1; else points[1].flags.samepoint=0; \
   points[1].flags.no_left = points[1].flags.no_right = 0; \
	points[1].left.x = x##b;   \
	points[1].left.y= y##b;   \
   points[1].left.vertex_number = b-1; \
	points[1].right.x = x##e;   \
	points[1].right.y= y##e;   \
   points[1].right.vertex_number = e-1; \
	if( c==f ) points[2].flags.samepoint=1; else points[2].flags.samepoint=0; \
	if( c ) { points[2].left.x = x##c;   \
	          points[2].left.y= y##c;      \
	          points[2].left.vertex_number = c-1; \
	          points[2].flags.no_left = 0;\
	} else points[2].flags.no_left = 1;\
	if( f ) { points[2].right.x = x##f;   \
	          points[2].right.y= y##f;    \
	          points[2].right.vertex_number = f-1; \
	          points[2].flags.no_right = 0;\
	} else points[2].flags.no_right = 1;\
	}

#define SCALE_SHIFT 0

static void PlotArbitrary( Image dest
						, Image source
						, int32_t x1, int32_t y1
						, int32_t x2, int32_t y2
						, int32_t x3, int32_t y3
						, int32_t x4, int32_t y4
						, uint32_t alpha
						, uint32_t mode
						, uint32_t param1
						, uint32_t param2
						, uint32_t param3
						)
{
	// x1, y1 define the position of the upper left corner of the image
	// x2, y2 define the position of the corner to the right of the upper left
	// x3, y3 define the position of the corner down from x2, y2
	// x4, y4 define the position left of the corner at x3, y3
	int lines = 0;
   int cols  = 0;
	struct {
		struct {
			int samepoint : 1;
			int no_left : 1;
			int no_right : 1;
		} flags;
		struct {
			int32_t x, y;
			int vertex_number; // 0, 1, 2, 3
		}left;
		struct {
			int32_t x, y;
			int vertex_number; // 0, 1, 2, 3
		} right;
	} points[3];

	struct {
		int x, y;
	} verts[4];

	int x0 = 0;
	int y0 = 0; // needed cause macro isn't the smartest...
#ifdef DEBUG_TIMING
	//lprintf( "-- Begin Setup" );
#endif

   verts[0].x = 0;
   verts[0].y = 0;
   verts[1].x = source->real_width << SCALE_SHIFT;
   verts[1].y = 0;
   verts[2].x = source->real_width << SCALE_SHIFT;
   verts[2].y = source->real_height << SCALE_SHIFT;
   verts[3].x = 0;
	verts[3].y = source->real_height << SCALE_SHIFT;
#ifdef DEBUG_TIMING
	{
		//lprintf( WIDE("points : %d,%d %d,%d %d,%d %d,%d"), x1, y1, x2, y2, x3, y3, x4, y4 );
	}
#endif

	if( y1 < y2 )
	{
		if( y1 < y3 )
		{
			if( y1 < y4 )
			{
				// y1 is the least
				if( ( y3 > y2 ) && ( y3 > y4 ) )
				{
					SET_POINTS( 1, 4, 3, 1, 2, 3 );
				}
				else if( y3 == y2 )
				{
					SET_POINTS( 1, 4, 3, 1, 2, 0 );
				}
				else if( y3 == y4 )
				{
					SET_POINTS( 1, 4, 0, 1, 2, 3 );
				}
				else
				{
					lprintf( WIDE("Invalid configuration ( convex? )") );
				}
			}
			else if( y4 < y1 )
			{
				// y4 is the least
				if( ( y2 > y3 ) && ( y2 > y1 ) )
				{
					SET_POINTS( 4, 3, 2, 4, 1, 2 );
				}
				else if( y2 == y3 )
				{
					SET_POINTS( 4, 3, 0, 4, 1, 2 );
				}
				else if( y2 == y1 )
				{
					SET_POINTS( 4, 3, 2, 4, 1, 0 );
				}
				else if( y3 == y2 )
				{
					SET_POINTS( 4, 3, 0, 4, 1, 2 );
				}
				else if( y2 == y1 )
				{
					SET_POINTS( 4, 3, 2, 4, 1, 0 );
				}
				else
				{
					lprintf( WIDE("Invalid configuration ( convex? )") );
				}

			}
			else // y4 == y1 and these are the least
			{
				if( y2 > y3 )
				{
					SET_POINTS( 4, 3, 2, 1, 2, 0 );
				}
				else if( y3 > y2 )
				{
					SET_POINTS( 4, 3, 0, 1, 2, 3 );
				}
				else
				{
					SET_POINTS( 4, 3, 0, 1, 2, 0 );
				}
			}
		}
		else if( y3 < y1 )
		{
			if( y3 < y4 )
			{
				// y3 is the least
				SET_POINTS( 3, 2, 1, 3, 4, 1 );
				//SET_POINTS( 3, 2, 1, 3,
			}
			else if( y4 < y3 )
			{
				// y4 is the least
				//if( ( y2 > y1 ) && ( y2 > y3 ) )
				SET_POINTS( 4, 3, 2, 4, 1, 2 );
				//else if(
			}
			else
			{
				// y3 and y4 are both least...
				if( y2 == y1 )
				{
					SET_POINTS( 3, 2, 0, 4, 1, 0 );
				}
				else if( y2 > y1 )
				{
					SET_POINTS( 3, 2, 0, 4, 1, 2 );
				}
				else
				{
					SET_POINTS( 3, 2, 1, 4, 1, 0 );
				}
			}
		}
		else // y1 == y3
		{
			if( y2 < y1 )
			{
            SET_POINTS( 2, 3, 4, 2, 1, 4 );
			}
			else if( y4 < y1 )
			{
            SET_POINTS( 2, 3, 4, 2, 1, 4 );
			}
			else
				lprintf( WIDE("Invalid Configuration!") );
		}
	}
	else if( y2 < y1 )
	{
		if( y2 < y3 )
		{
			if( y2 < y4 )
			{
				if( ( y4 > y1 ) && ( y4 > y3 ) )
				{
					SET_POINTS( 2, 1, 4, 2, 3, 4 );
				}
				else if( y4 == y1 )
				{
					SET_POINTS( 2, 1, 0, 2, 3, 4 );
				}
				else if( y4 == y3 )
				{
					SET_POINTS( 2, 1, 4, 2, 3, 0 );
				}
				else
					lprintf( WIDE("Invalid configuration!") );
				// y2 is the least
			}
			else if( y4 <= y2 )
			{
				lprintf( WIDE("Invalid configuration!") );
			}
		}
		else if( y3 < y2 )
		{
			if( y3 == y4 )
			{
            SET_POINTS( 3, 2, 0, 4, 1, 0 );
			}
			else if( ( y1 > y2 ) && ( y1 > y4 ) )
			{
				SET_POINTS( 3, 2, 1, 3, 4, 1 );
			}
			else if( y1 == y2 )
			{
				SET_POINTS( 3, 2, 0, 3, 4, 1 );
			}
			else if( y1 == y4 )
			{
				SET_POINTS( 3, 2, 1, 3, 4, 0 );
			}
			else
				lprintf( WIDE("Invalid configuration!") );
			// y3 is the least
		}
		else //if( y2 == y3 )
		{
			if( y4 > y1 )
			{
				SET_POINTS( 2, 1, 4, 3, 4, 0 );
			}
			else if( y4 < y1 )
			{
				SET_POINTS( 2, 1, 0, 3, 4, 1 );
			}
			else
			{
				SET_POINTS( 2, 1, 0, 3, 4, 0 );

			}
		}
      /*
		else
		{
			// y4 is the least
			if( ( y2 > y3 ) && ( y2 > y1 ) )
			{
				SET_POINTS( 4, 3, 2, 4, 1, 2 );
			}
			else if( y2 == y3 )
			{
				SET_POINTS( 4, 3, 0, 4, 1, 2 );
			}
			else if( y2 == y1 )
			{
				SET_POINTS( 4, 3, 2, 4, 1, 0 );
			}
         else
            lprintf( "Invalid configuration!" );
				}
            */
	}
	else // y1 == y2
	{
		if( y1 < y3 )
		{
			if( y3 > y4 )
			{
				SET_POINTS( 1, 4, 3, 2, 3, 0 );
			}
			else if( y3 < y4 )
			{
				SET_POINTS( 1, 4, 0, 2, 3, 4 );
			}
			else // we're square!
			{
				SET_POINTS( 1, 4, 0, 2, 3, 0 );
			}
		}
		else if( y3 < y1 )
		{
			if( y3 == y4 )
			{
            SET_POINTS( 3,2,0,4,1,0 );
			}
         else
			// y3 is less than both y2 and y1
			if( y3 < y4 )
			{
            SET_POINTS( 3,2,1,3,4,1 );
            // y3 is least
			}
			else if( y4 < y3 )
			{
            SET_POINTS( 4,3,2,4,1,2 );
				// y4 is least
			}

		}
		else
         lprintf( WIDE("Invalid configuration.. y1, y2, and y3 all equal") );
	}

#ifdef DEBUG_TIMING
	//lprintf( "-- End Setup" );
#endif
	{
#ifdef DEBUG_TIMING
		int loops[20];
#endif
		int output = 0;
		struct {
			int idx;
			int curx, cury; // cury is common between left and right.
			struct {
				int x, y;
            int out, image; // delta output vs delta image
			} del;
			int incx, incy;
			int err;
			int err2; // accumulator for delta output vs delta image
			struct {
				int curx, cury;
				struct {
					int x, y;
				} del;
            int incx, incy;
            int err;
			} image;
		} right, left;

		struct {
			int curx, cury; // current pixel on stride from left.image. to right.image.
			struct {
				int x, y;
			} del; // current delta of left.image.current to right.image curre
			int err;
         int incx, incy;
		} image;

		struct {
			// delta of this are from min point to max point
			// and left.curx to right.curx
         // but
			int curx, cury;
			struct {
				int out, image; // except in this case it's
			} del;
         int incout;
         int err;
		} out;

#ifdef DEBUG_TIMING
		for( output = 0; output<20;output++ ) loops[output] = 0;
#endif
		output = 0;
		//out.cury = points[0].left.y;
		left.idx = 0;
		right.idx = 0;

		// need to cover the length of an edge in the Y length of the screen.
		// this is a fixed rule, since we scan always each Y of the screen and the
		// across on each X of the screen.  the X is determined from
		//     left.image.curx, left.image.cury to right.image.curx, right.image.cury
      //     which is spanned in the delta from left.curx to right.curx.

		//--------------------------
		left.curx = points[left.idx].left.x;
		left.cury = points[left.idx].left.y; // really this is common... but for properness I should set it.
		//--------------------------
		right.curx = points[right.idx].right.x;
		right.cury = points[right.idx].right.y; // really this is common... but for properness I should set it.
		//--------------------------
		//lprintf( "..." );
		do
		{
			//if( ( left.cury - points[left.idx-1].left.y ) == 10 )
			//   DebugBreak();
			//lprintf( "Begin line." );
			lines++;
			if( left.idx > 0 )
			{
				out.curx = left.curx;
				out.cury = left.cury;
				out.del.out = right.curx - left.curx;
				if( out.del.out < 0 )
				{
					out.del.out = -out.del.out;
               out.incout = -1;
				}
				else
               out.incout = 1;

				image.curx = left.image.curx;
				image.cury = left.image.cury;

				//--------------------------
				image.del.x = right.image.curx - left.image.curx;
				if( image.del.x < 0 )
				{
					image.del.x = -image.del.x;
					image.incx = -1;
				}
				else
					image.incx = 1;

				//--------------------------
				image.del.y = right.image.cury - left.image.cury;
				if( image.del.y < 0 )
				{
					image.del.y = -image.del.y;
					image.incy = -1;
				}
				else
					image.incy = 1;
				image.err = (-image.del.x)/2;

				//--------------------------
				// this loop iterates the image by one pixel...
				// but I don't really need THIS loop... so what's the next?
				// the iterator of curx's on the image, according to the step of
				// this pixel vs the whole line...
				//printf( "Line: %d  (%d-%d)\n", out.cury, left.image.curx, right.image.curx );
				//if( out.cury == 82 )
				//   DebugBreak();
				if( image.del.x < image.del.y )
				{
					out.del.image = image.del.y;
					{
						out.err = (-out.del.image)/2;
						image.err = (-image.del.y)/2;
						if( out.curx < dest->x )
                     out.curx = dest->x;
						if( right.curx > dest->width )
                     right.curx = dest->width;
						if( out.cury >= dest->y && out.cury < dest->height )
						while( out.curx <= right.curx )
						{
#ifdef DEBUG_TIMING
							loops[0]++;
#endif
						   //if( //( out.curx >= dest->x ) && ( out.curx < (dest->x + dest->width) )
								//( out.cury >= dest->y ) && ( out.cury < (dest->y + dest->height) )
							//  )
							{
								int cx, cy;

#ifdef OUTPUT_IMAGE
								if( ( (cx=image.curx>>SCALE_SHIFT) < source->width ) &&
									( (cy=image.cury>>SCALE_SHIFT) < source->height ) )
								{
									CDATA c = *IMG_ADDRESS( source, cx, cy );
									CDATA *po = IMG_ADDRESS(dest,out.curx,out.cury);
									int alpha1 = AlphaVal(c);
									*po = DOALPHA2( *po, c, alpha1 ) ;
									output++;
								}
#endif
							}
							//plot( dest, out.curx, out.cury
							//	 , getpixel( source, image.curx >> SCALE_SHIFT, image.cury >> SCALE_SHIFT )
								  //, Color( image.curx*4, image.cury*4, 0 )
							//	 );
							out.err += out.del.image;
							out.curx++;
							while( out.curx <= right.curx && out.err > 0 )
							{
#ifdef DEBUG_TIMING
								loops[1]++;
#endif
								out.err -= out.del.out;
								{ // iterate the image one step. (ie out.image++ )
									image.err += image.del.x;
									image.cury += image.incy;
									while( image.err > 0 )
									{
#ifdef DEBUG_TIMING
										loops[2]++;
#endif
										image.err -= image.del.y;
										image.curx += image.incx;
									}
								}
							}
						}
					}
				}
				else
				{
					out.del.image = image.del.x;
					{
						out.err = (-out.del.image)/2;
						image.err = (-image.del.x)/2;
						//lprintf( "plot..." );
						cols = 0;
						if( out.curx < dest->x )
                     out.curx = dest->x;
						if( right.curx > dest->width )
							right.curx = dest->width;
						if( out.cury >= dest->y && out.cury < dest->height )

						while( out.curx <= right.curx )
						{
							int cx, cy; // = image.curx >> SCALE_SHIFT, cy = image.cury >> SCALE_SHIFT;
#ifdef DEBUG_TIMING
							loops[3]++;
#endif
							cols++;
							//printf( "c %d,", image.curx );
						   //if( //( out.curx >= dest->x ) && ( out.curx < (dest->x + dest->width) )
								//&&
								//( out.cury >= dest->y ) && ( out.cury < (dest->y + dest->height) ) )
							{
#if defined( OUTPUT_IMAGE )
								if( ( (cx=image.curx>>SCALE_SHIFT) < source->width ) &&
									( (cy=image.cury>>SCALE_SHIFT) < source->height ) )
								{
									CDATA c;
									CDATA *po;
									int alpha1;
									c = *IMG_ADDRESS( source, cx, cy );
									po = IMG_ADDRESS(dest,out.curx,out.cury);
									alpha1 = AlphaVal(c);
									*po = DOALPHA2( *po, c, alpha1 ) ;
									output++;
								}
#endif
							}
							//plot( dest, out.curx, out.cury
							//	 , getpixel( source, image.curx >> SCALE_SHIFT, image.cury >> SCALE_SHIFT )
								  //, Color( image.curx*4, image.cury*4, 0 )
							//	 );
							out.err += out.del.image;
							out.curx++;
							while( out.curx <= right.curx && out.err > 0 )
							{
#ifdef DEBUG_TIMING
								loops[4]++;
#endif
								out.err -= out.del.out;
								{ // iterate the image one step. (ie out.image++ )
									image.err += image.del.y;
									image.curx += image.incx;
									while( image.err > 0 )
									{
#ifdef DEBUG_TIMING
										loops[5]++;
#endif
										image.err -= image.del.x;
										image.cury += image.incy;
									}
								}
							}
						}
						//lprintf( "end plot... %d", cols );
					}
				}

				//printf( "\n" );
			} // if ( left.idx > 0 )



			//--------------
			// check to see if we need to change the left or right
			// segments...
			if( left.idx )
			{
				//---------------
				// increment output X on left and right side.
				// always step by 1 Y therefore we always add the same del.x...
				//

				{
					// step the output coordinate....
					left.err += left.del.x;
					/* incy will always be 1... perhaps effectively zero...*/
					left.cury += left.incy;
					while( left.err > 0 )
					{
#ifdef DEBUG_TIMING
						loops[6]++;
#endif
						if( !left.del.y )
		                     DebugBreak();
						left.err -= left.del.y;
						left.curx += left.incx;
					}
				}

            // step increment output vs image...
				left.err2 += left.del.image;
				if( left.image.del.x < left.image.del.y )
				{
					while( left.err2 > 0 )
					{
#ifdef DEBUG_TIMING
						loops[7]++;
#endif
						left.err2 -= left.del.out; // the larger distance to go on the line.
						left.image.err += left.image.del.x;
						// increment the longer distance by a unit increment.
						left.image.cury += left.image.incy;
						while( left.image.err > 0 )
						{
#ifdef DEBUG_TIMING
							loops[8]++;
#endif
							left.image.err -= left.del.out;
							left.image.curx += left.image.incx;
							//printf( "," );
						}
					}
				}
				else // left.image.del.x > left.image.del.y
				{
					while( left.err2 > 0 )
					{
#ifdef DEBUG_TIMING
						loops[9]++;
#endif
						left.err2 -= left.del.out; // the larger distance to go on the line.
						left.image.err += left.image.del.y;
						// incement the longer distance by a unit distance.
						left.image.curx += left.image.incx;
						//DebugBreak();
						//printf( "." );
						while( left.image.err > 0 )
						{
#ifdef DEBUG_TIMING
							loops[10]++;
#endif
							left.image.err -= left.image.del.x;
							left.image.cury += left.image.incy;
						}
					}
				}
			} // if left.cury == final y
			if( (left.cury == points[left.idx].left.y) )
			{
            //DebugBreak();
				if( points[left.idx+1].flags.no_left || ( left.idx == 2 ) )
				{
					// done!
					break;
				}
				else
				{
					left.curx = points[left.idx].left.x;
					left.cury = points[left.idx].left.y; // really this is common... but for properness I should set it.
					left.image.curx = verts[points[left.idx].left.vertex_number].x;
					left.image.cury = verts[points[left.idx].left.vertex_number].y;
					left.del.x = points[left.idx+1].left.x - left.curx;
					if( left.del.x < 0 )
					{
						left.del.x = -left.del.x;
						left.incx = -1;
					}
					else
						left.incx = 1;

					left.del.y = points[left.idx+1].left.y - left.cury;
					if( left.del.y < 0 )
					{
						left.del.y = -left.del.y;
						left.incy = -1;
					}
					else
						left.incy = 1;
					left.del.out = left.del.y;

					left.image.del.x = verts[points[left.idx+1].left.vertex_number].x - left.image.curx;
					if( left.image.del.x < 0 )
					{
						left.image.del.x = -left.image.del.x;
						left.image.incx = -1;
					}
					else if( left.image.del.x > 0 )
						left.image.incx = 1;
					else
						left.image.incx = 0;

					left.image.del.y = verts[points[left.idx+1].left.vertex_number].y - left.image.cury;
					if( left.image.del.y < 0 )
					{
						left.image.del.y = -left.image.del.y;
						left.image.incy = -1;
					}
					else if( left.image.del.y > 0 )
						left.image.incy = 1;
					else
						left.image.incy = 0;

					left.err = (-left.del.x)/2; // always incremented by del.y ( +1 y ) therefore comparison starts at /2 x;

					if( left.image.del.x > left.image.del.y )
					{
						left.del.image = left.image.del.x;
						left.err2 = ( -left.image.del.x ) / 2;
						left.image.err = (-left.image.del.y)/2;
					}
					else
					{
						left.del.image = left.image.del.y;
						left.err2 = ( -left.image.del.y ) / 2;
						left.image.err = (-left.image.del.x)/2;
					}
					left.idx++;
				}
			}

			if( right.idx ) // not at a point boundry ( cury != nexty )
			{
            // step the output coordinate....
				right.err += right.del.x;
				/* incy will always be 1... perhaps effectively zero...*/
				right.cury += right.incy;
				while( right.err > 0 )
				{
#ifdef DEBUG_TIMING
					loops[11]++;
#endif
					right.err -= right.del.y;
					right.curx += right.incx;
				}

				right.err2 += right.del.image;
				if( right.image.del.x < right.image.del.y )
				{
					while( right.err2 > 0 )
					{
#ifdef DEBUG_TIMING
						loops[12]++;
#endif
						right.err2 -= right.del.out; // the larger distance to go on the line.
						right.image.err += right.image.del.x;
						// increment the longer distance by a unit increment.
						right.image.cury += right.image.incy;
						while( right.image.err > 0 )
						{
#ifdef DEBUG_TIMING
							loops[13]++;
#endif
							right.image.err -= right.image.del.y;
							right.image.curx += right.image.incx;
						}
					}
				}
				else // right.image.del.x < right.image.del.y
				{
					while( right.err2 > 0 )
					{
#ifdef DEBUG_TIMING
						loops[14]++;
#endif
						right.err2 -= right.del.out; // the larger distance to go on the line.
						right.image.err += right.image.del.y;
						// increment the longer distance by a unit increment.
						right.image.curx += right.image.incx;
						while( right.image.err > 0 )
						{
#ifdef DEBUG_TIMING
							loops[15]++;
#endif
							right.image.err -= right.image.del.x;
							right.image.cury += right.image.incy;
						}
					}
				}
			} // if right.cury == destination point.

			if( (right.cury == points[right.idx].right.y) )
			{
            //DebugBreak();
				if( points[right.idx+1].flags.no_right || ( right.idx == 2 ) )
				{
					// done!
					break;
				}
				else
				{
					right.curx = points[right.idx].right.x;
					right.cury = points[right.idx].right.y;
					right.image.curx = verts[points[right.idx].right.vertex_number].x;
					right.image.cury = verts[points[right.idx].right.vertex_number].y;
					right.del.x = points[right.idx+1].right.x - right.curx;
					if( right.del.x < 0 )
					{
						right.del.x = -right.del.x;
						right.incx = -1;
					}
					else
						right.incx = 1;

					right.del.y = points[right.idx+1].right.y - points[right.idx].right.y;
					if( right.del.y < 0 )
					{
						right.del.y = -right.del.y;
						right.incy = -1;
					}
					else
						right.incy = 1;
					right.del.out = right.del.y;

					right.image.del.x = verts[points[right.idx+1].right.vertex_number].x - right.image.curx;
					if( right.image.del.x < 0 )
					{
						right.image.del.x = -right.image.del.x;
						right.image.incx = -1;
					}
					else if( right.image.del.x > 0 )
						right.image.incx = 1;
					else
						right.image.incx = 0;

					right.image.del.y = verts[points[right.idx+1].right.vertex_number].y - right.image.cury;
					if( right.image.del.y < 0 )
					{
						right.image.del.y = -right.image.del.y;
						right.image.incy = -1;
					}
					else if( right.image.del.y > 0 )
						right.image.incy = 1;
					else
						right.image.incy = 0;

					right.err = (-right.del.x)/2; // always incremented by del.y ( +1 y ) therefore comparison starts at /2 x;

					if( right.image.del.x > right.image.del.y )
					{
						right.del.image = right.image.del.x;
						right.err2 = ( -right.image.del.x ) / 2;
						right.image.err = (-right.image.del.y)/2;
					}
					else
					{
						right.del.image = right.image.del.y;
						right.err2 = ( -right.image.del.y ) / 2;
						right.image.err = (-right.image.del.x)/2;
					}
					right.idx++;
				}
			}
         //lprintf( "end line." );
		}
		while( left.idx < 3 && right.idx < 3 );
#ifdef DEBUG_TIMING
		lprintf( "lines : %" _32f " cols: %" _32f " output: %d", lines, cols, output );
		{
			char buf[256];
			int ofs = 0;
			int n;
			for( n = 0; n < 20; n++ )
				ofs += snprintf( buf + ofs, sizeof( buf ) - ofs, "L(%d)=%d ", n, loops[n] );
			lprintf( buf );
		}
#endif
	}
}

static void (CPROC *SavePortion )( PSPRITE_METHOD psm, uint32_t x, uint32_t y, uint32_t w, uint32_t h );


 void  SetSavePortion ( void (CPROC*_SavePortion )( PSPRITE_METHOD psm, uint32_t x, uint32_t y, uint32_t w, uint32_t h ) )
{
   SavePortion = _SavePortion;
}


static void TranslatePoints( Image dest, PSPRITE sprite )
{
	int x1, y1;
	int x2, y2;
	int x3, y3;
	int x4, y4;
	_POINT result;
	_POINT tmp;
	static uint32_t lock;
	static PTRANSFORM transform;
	int32_t xd, yd;
	//lprintf( "-- Begin Transform" );
	while( LockedExchange( &lock, 1 ) ) Relinquish();
	if( !transform )
		transform = CreateNamedTransform( NULL );
	xd = sprite->curx;// * sprite->scalex / (RCOORD)0x10000;
	yd = sprite->cury;// * sprite->scaley / (RCOORD)0x10000;
	TranslateCoord( dest, &xd, &yd );

	Translate( transform
				, (RCOORD)xd
				, (RCOORD)yd
				, (RCOORD)0 );
	//lprintf( WIDE("angle = %ld"), sprite->angle );
	Scale( transform, sprite->scalex / (RCOORD)0x10000, sprite->scaley / (RCOORD)0x10000, 0 );
#ifdef DEBUG_TIMING
	//lprintf( WIDE("angle = %ld"), sprite->angle );
#endif
	RotateAbs( transform, (RCOORD)0, (RCOORD)0, (RCOORD)sprite->angle );
   //Scale( transform, 1, 1, 0 );
	tmp[0] = (0) - sprite->hotx;
	tmp[1] = (0) - sprite->hoty;
	tmp[2] = 0;
	Apply( transform, result, tmp );
	sprite->minx = sprite->maxx = x1 = (int)result[0]; // + sprite->curx;
	sprite->miny = sprite->maxy = y1 = (int)result[1]; // + sprite->cury;
	tmp[0] = (sprite->image->real_width) - sprite->hotx;
	tmp[1] = (0) - sprite->hoty;
	tmp[2] = 0;
	Apply( transform, result, tmp );
	x2 = (int)result[0]; // + sprite->curx;
	if( x2 > sprite->maxx ) sprite->maxx = x2;
	if( x2 < sprite->minx ) sprite->minx = x2;
	y2 = (int)result[1]; // + sprite->cury;
	if( y2 > sprite->maxy ) sprite->maxy = y2;
	if( y2 < sprite->miny ) sprite->miny = y2;
	tmp[0] = (sprite->image->real_width) - sprite->hotx;
	tmp[1] = (sprite->image->real_height) - sprite->hoty;
	tmp[2] = 0;
	Apply( transform, result, tmp );
	x3 = (int)result[0]; // + sprite->curx;
	if( x3 > sprite->maxx ) sprite->maxx = x3;
	if( x3 < sprite->minx ) sprite->minx = x3;
	y3 = (int)result[1]; // + sprite->cury;
	if( y3 > sprite->maxy ) sprite->maxy = y3;
	if( y3 < sprite->miny ) sprite->miny = y3;
	tmp[0] = (0) - sprite->hotx;
	tmp[1] = (sprite->image->real_height) - sprite->hoty;
	tmp[2] = 0;
	Apply( transform, result, tmp );
	x4 = (int)result[0]; // + sprite->curx;
	if( x4 > sprite->maxx ) sprite->maxx = x4;
	if( x4 < sprite->minx ) sprite->minx = x4;
	y4 = (int)result[1]; // + sprite->cury;
	if( y4 > sprite->maxy ) sprite->maxy = y4;
	if( y4 < sprite->miny ) sprite->miny = y4;
	lock = 0;
   //DestroyTransform( transform );
	//lprintf( "-- End Transform" );
	if( SavePortion && sprite->pSpriteMethod )
	{
		/*
		 * can self check the bounds of the update portion method, and update that...
		 * probably, however, that would result in sprite tear.
       *
		if( !( ( sprite->minx > (sprite->pSpriteMethod->x + sprite->pSpriteMethod->w) )
				&&( sprite->maxx < (sprite->pSpriteMethod->x) )
				&&( sprite->miny > (sprite->pSpriteMethod->y + sprite->pSpriteMethod->h) )
				&&( sprite->maxy < (sprite->pSpriteMethod->y) ) ) )
		*/
		{
			// could also save the current rectangle, set the current as the clipping rectangle
			// save it, restore it, and restore the original clipping rectangle...
			// wow what magic this is...
#ifdef DEBUG_TIMING
			//lprintf( "save portion? ");
#endif
			SavePortion( sprite->pSpriteMethod
						  , sprite->minx, sprite->miny
						  , (sprite->maxx - sprite->minx) + 1
						  , (sprite->maxy - sprite->miny) + 1 );
#ifdef DEBUG_TIMING
			//lprintf( "saved.." );
#endif
		}
	}
	if( !(dest->flags & IF_FLAG_FINAL_RENDER ) )
	{
#ifdef DEBUG_TIMING
		lprintf( "Output arbitrary" );
#endif
		//lprintf( "plot arbitraty..." );
		PlotArbitrary( dest, sprite->image
						 , x1, y1
						 , x2, y2
						 , x3, y3
						 , x4, y4
						 , 0
						 , BLOT_COPY
						 , 0, 0, 0 );
		//lprintf( "done plott..." );
#ifdef DEBUG_TIMING
		lprintf( "arbitrary out" );
#endif
	}
	else
	{
			VECTOR v[2][4];
			float texture_v[4][2];
			int vi = 0;
			//TranslateCoord( dest, &xd, &yd );

			v[vi][0][0] = x1;
			v[vi][0][1] = y1;
			v[vi][0][2] = 0.0;

			v[vi][1][0] = x2;
			v[vi][1][1] = y2;
			v[vi][1][2] = 0.0;

			v[vi][2][0] = x4;
			v[vi][2][1] = y4;
			v[vi][2][2] = 0.0;

			v[vi][3][0] = x3;
			v[vi][3][1] = y3;
			v[vi][3][2] = 0.0;


			while( dest && dest->pParent )
			{
				if( dest->transform )
				{
					Apply( dest->transform, v[1-vi][0], v[vi][0] );
					Apply( dest->transform, v[1-vi][1], v[vi][1] );
					Apply( dest->transform, v[1-vi][2], v[vi][2] );
					Apply( dest->transform, v[1-vi][3], v[vi][3] );
					vi = 1-vi;
				}
				dest = dest->pParent;
			}
			if( dest->transform )
			{
				Apply( dest->transform, v[1-vi][0], v[vi][0] );
				Apply( dest->transform, v[1-vi][1], v[vi][1] );
				Apply( dest->transform, v[1-vi][2], v[vi][2] );
				Apply( dest->transform, v[1-vi][3], v[vi][3] );
				vi = 1-vi;
			}

			scale( v[vi][0], v[vi][0], l.scale );
			scale( v[vi][1], v[vi][1], l.scale );
			scale( v[vi][2], v[vi][2], l.scale );
			scale( v[vi][3], v[vi][3], l.scale );

			{
				float x_size, x_size2, y_size, y_size2;
				int xs, ys, ws, hs;
				Image topmost_parent ;
				xs = 0;
				ys = 0;
				ws = sprite->image->width;
				hs = sprite->image->height;
				for( topmost_parent = sprite->image; topmost_parent && topmost_parent->pParent; topmost_parent = topmost_parent->pParent )
				{
					xs += topmost_parent->real_x;
					ys += topmost_parent->real_y;
				}
				x_size = (float) xs/ (float)topmost_parent->width;
				x_size2 = (float) (xs+ws)/ (float)topmost_parent->width;
				y_size = (float) ys/ (float)topmost_parent->height;
				y_size2 = (float) (ys+hs)/ (float)topmost_parent->height;


				ReloadOpenGlTexture( sprite->image, 0 );
				//EnableShader( GetShader( "Simple Texture", NULL ), v[vi], topmost_parent->glActiveSurface, texture_v );
			glBindTexture(GL_TEXTURE_2D, sprite->image->glActiveSurface);				// Select Our Texture
			glBegin(GL_TRIANGLE_STRIP);
			glTexCoord2f(x_size, y_size); glVertex3fv(v[vi][0]);	// Bottom Left Of The Texture and Quad
			glTexCoord2f(x_size2, y_size); glVertex3fv(v[vi][1]);	// Bottom Right Of The Texture and Quad
			glTexCoord2f(x_size, y_size2); glVertex3fv(v[vi][2]);	// Top Left Of The Texture and Quad
			glTexCoord2f(x_size2, y_size2); glVertex3fv(v[vi][3]);	// Top Right Of The Texture and Quad
			glEnd();
		}
	}
}



/* rotate_scaled_sprite:
 *  Draws a sprite image onto a bitmap at the specified position, rotating 
 *  it by the specified angle. The angle is a fixed point 16.16 number in 
 *  the same format used by the fixed point trig routines, with 256 equal 
 *  to a full circle, 64 a right angle, etc. This function can draw onto
 *  both linear and mode-X bitmaps.
 */
  void  rotate_scaled_sprite (ImageFile *bmp, SPRITE *sprite, fixed angle, fixed scale_width, fixed scale_height)
{
   //lprintf( "rotate_scaled_sprite..." );
#ifdef DEBUG_TIMING
	//lprintf( WIDE("input angle = %ld"), angle );
#endif
	sprite->angle = (float)(( ( 2 * 3.14159268 ) * angle ) / 0x100000000LL);
#ifdef DEBUG_TIMING
	//lprintf( WIDE("output angle si %g"), sprite->angle );
#endif
	sprite->scalex = scale_width;
	sprite->scaley = scale_height;
	TranslatePoints( bmp, sprite );
}

/* rotate_sprite:
 *  Draws a sprite image onto a bitmap at the specified position, rotating 
 *  it by the specified angle. The angle is a fixed point 16.16 number in 
 *  the same format used by the fixed point trig routines, with 256 equal 
 *  to a full circle, 64 a right angle, etc. This function can draw onto
 *  both linear and mode-X bitmaps.
 */
  void  rotate_sprite (ImageFile *bmp, SPRITE *sprite, fixed angle)
{
	rotate_scaled_sprite(bmp, sprite, angle, 0x10000, 0x10000 );
}


IMAGE_NAMESPACE_END

// $Log: sprite.c,v $
// Revision 1.9  2005/04/05 11:56:04  panther
// Adding sprite support - might have added an extra draw callback...
//
// Revision 1.8  2004/06/21 07:47:13  d3x0r
// Account for newly moved structure files.
//
// Revision 1.7  2003/03/25 08:45:51  panther
// Added CVS logging tag
//
