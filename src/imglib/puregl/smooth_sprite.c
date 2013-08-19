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
#define IMAGE_LIBRARY_SOURCE
#include <stdhdrs.h>
#include <stdio.h>
#include <string.h>
#define LIBRARY_DEF
#include <sharemem.h>
#include <vectlib.h>
#include <imglib/imagestruct.h>
#include "blotproto.h"
#include "image.h"
#include "sprite_local.h"



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

#define SCALE_SHIFT 1

//cpg26dec2006 c:\work\sack\src\imglib\smooth_sprite.c(67): Warning! W202: Symbol 'SampleLine' has been defined, but not referenced
//cpg26dec2006 static void SampleLine( )
//cpg26dec2006 {
//cpg26dec2006 }

static void PlotArbitrary( Image dest
						, Image source_image
						, S_32 x1, S_32 y1
						, S_32 x2, S_32 y2
						, S_32 x3, S_32 y3
						, S_32 x4, S_32 y4
						, _32 alpha
						, _32 mode
						, _32 param1
						, _32 param2
						, _32 param3
						)
{
	// x1, y1 define the position of the upper left corner of the image
	// x2, y2 define the position of the corner to the right of the upper left
	// x3, y3 define the position of the corner down from x2, y2
	// x4, y4 define the position left of the corner at x3, y3
	int lines = 0;
	int cols  = 0;
	static struct {
		// if the screen iterates less than the image
		// or even for multi pixel accuracy, the colors
		// should be summed and all samples divided to
		// get the resulting color;
		// However, colors should be treated subtarcted
		// by 128 for each sample.  and then divided
		// and then shifted back up +128.
		// ( sample - (128*samples) ) / samples ) + 128;
#define CONVERT_SAMPLE(channel,n) ( workspace.samples[n]?( ( workspace.channel[n] /*- (128*workspace.samples[n])*/ ) / workspace.samples[n] ):0 )
//#define CONVERT_SAMPLE(channel,n) ( ( ( workspace.channel[n] - (128*workspace.samples[n]) ) / workspace.samples[n]/2 ) + 128 )
#define DC_COLOR_BIAS 128
      S_16 red[4096];
      S_16 blue[4096];
      S_16 green[4096];
		S_16 alpha[4096];
		_32 samples[4096];
      // when it comes time to output, this should be saved.
		_32 min_sample;
      _32 max_sample;
      //_32 used;
	} workspace;
   static _32 used;
	struct {
		struct {
			int samepoint : 1;
			int no_left : 1;
			int no_right : 1;
		} flags;
		struct {
			S_32 x, y;
			int vertex_number; // 0, 1, 2, 3
		}left;
		struct {
			S_32 x, y;
			int vertex_number; // 0, 1, 2, 3
		} right;
	} points[3];

	struct {
		int x, y;
	} verts[4];

	//cpg26dec2006 c:\work\sack\src\imglib\smooth_sprite.c(133): Warning! W200: 'y0' has been referenced but never assigned a value
   //cpg26dec2006 c:\work\sack\src\imglib\smooth_sprite.c(133): Warning! W200: 'x0' has been referenced but never assigned a value

	int x0, y0; // needed cause macro isn't the smartest...
//	int x0 = 0, y0 = 0; // needed cause macro isn't the smartest...
   x0 = y0 = 0;
#ifdef DEBUG_TIMING
	lprintf( "-- Begin Setup" );
#endif
	if( !source_image->image || ( source_image->flags & IF_FLAG_HIDDEN ) )
      return; // no actual image data... could also check ->flags & IF_FLAG_HIDDEN... should track 100%
	while( LockedExchange( &used, 1 ) )
      Relinquish();
   verts[0].x = 0;
   verts[0].y = 0;
   verts[1].x = source_image->real_width << SCALE_SHIFT;
   verts[1].y = 0;
   verts[2].x = source_image->real_width << SCALE_SHIFT;
   verts[2].y = source_image->real_height << SCALE_SHIFT;
   verts[3].x = 0;
	verts[3].y = source_image->real_height << SCALE_SHIFT;
#if 0
	{
		lprintf( WIDE("points : %d,%d %d,%d %d,%d %d,%d"), x1, y1, x2, y2, x3, y3, x4, y4 );
	}
#endif
	if( y1 < y2 || ( ( y3 < y2 ) && ( y3 < y1 ) ) || ( ( y4 < y2 ) && ( y4 < y1 ) ) )
	{
		if( y1 < y3 || ( y4 < y3 ) )
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
					DebugBreak();
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
			DebugBreak();
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
				DebugBreak();
				lprintf( WIDE("Invalid configuration!") );
			}
		}
		else if( y3 < y4 )
		{
			if( ( y1 > y2 ) && ( y1 > y4 ) )
			{
				SET_POINTS( 3, 2, 1, 3, 4, 1 );
			}
			else if( y1 == y2 )
			{
				SET_POINTS( 3, 2, 0, 3, 4, 1 );
			}
			else if( y1 == y4 )
			{
				if( y2 == y3 )
				{
					SET_POINTS( 2, 1, 0, 3, 4, 0 );
				}
				else
				{
					SET_POINTS( 3, 2, 1, 3, 4, 0 );
				}
			}
			else
			{
				DebugBreak();
				lprintf( WIDE("Invalid configuration!") );
			}
			// y3 is the least
		}
		else if( y4 < y3 )
		{ // y4 is the least
			SET_POINTS( 3, 2, 1, 3, 4, 1 );
		}
		else// if( y2 == y3 )
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
			// y3 is less than both y2 and y1
			if( y3 < y4 )
			{
				// y3 is least
            SET_POINTS( 3, 2, 1, 3, 4, 1 );
            DebugBreak();
			}
			else if( y4 < y3 )
			{
            SET_POINTS( 4, 3, 0, 4, 1, 2 );
				// y4 is least
            DebugBreak();
			}
			else
			{
            SET_POINTS( 4, 3, 0, 3, 1, 2 );
            DebugBreak();
			}
		}
		else
         lprintf( WIDE("Invalid configuration.. y1, y2, and y3 all equal") );
	}

#ifdef DEBUG_TIMING
	lprintf( WIDE("-- End Setup") );
#endif
	{
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
			struct {
				int curx, cury; // current pixel on stride from left.image. to right.image.
				struct {
					int x, y;
				} del; // current delta of left.image.current to right.image curre
				int err;
				int incx, incy;
			} image;
		} source;

		struct {
			// delta of this are from min point to max point
			// and left.curx to right.curx
         // but
			int curx, cury;
			struct {
				struct {
					int x, y;
				} out;
				struct {
					int x, y;
				} image; // except in this case it's
			} del;
			struct {
				int x, y;
			} err;
		} out;


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

		do
		{
			//if( ( left.cury - points[left.idx-1].left.y ) == 10 )
			//   DebugBreak();
			//lprintf( "Begin line." );
			lines++;
			{
				// clear all information collected about any line.
				MemSet( &workspace, 0, sizeof( workspace ) );
			}
			if( left.idx > 0 )
			{
				out.curx = left.curx;
				out.cury = left.cury;
				out.del.out.x = right.curx - left.curx;
				if( out.del.out.x < 0 )
               out.del.out.x = -out.del.out.x;

				source.image.curx = left.image.curx;
				source.image.cury = left.image.cury;

				//--------------------------
				source.image.del.x = right.image.curx - left.image.curx;
				if( source.image.del.x < 0 )
				{
					source.image.del.x = -source.image.del.x;
					source.image.incx = -1;
				}
				else
					source.image.incx = 1;

				//--------------------------
				source.image.del.y = right.image.cury - left.image.cury;
				if( source.image.del.y < 0 )
				{
					source.image.del.y = -source.image.del.y;
					source.image.incy = -1;
				}
				else
					source.image.incy = 1;
				source.image.err = (-source.image.del.x)/2;

				//--------------------------
				// this loop iterates the image by one pixel...
				// but I don't really need THIS loop... so what's the next?
				// the iterator of curx's on the image, according to the step of
				// this pixel vs the whole line...
				//printf( "Line: %d  (%d-%d)\n", out.cury, left.image.curx, right.image.curx );
				//if( out.cury == 82 )
				//   DebugBreak();
				if( source.image.del.x < source.image.del.y )
				{
					out.del.image.x = source.image.del.y;
					out.del.image.y = source.image.del.x;
					{
						out.err.x = (-out.del.image.x)/2;
						source.image.err = (-source.image.del.y)/2;
						while( out.curx <= right.curx )
						{
#ifdef OUTPUT_IMAGE
							if( ( out.curx >= dest->x ) && ( out.curx < (dest->x + dest->width) )
								&& ( out.cury >= dest->y ) && ( out.cury < (dest->y + dest->height) ) )
							{
								//CDATA *po = IMG_ADDRESS(dest,out.curx,out.cury);
								if( ( ( source.image.curx >> SCALE_SHIFT ) < source_image->width ) &&
								  ( ( source.image.cury >> SCALE_SHIFT ) < source_image->height ) )
								if( out.curx > 0 )
								{
									CDATA c = *IMG_ADDRESS( source_image, source.image.curx >> SCALE_SHIFT, source.image.cury >> SCALE_SHIFT );
									workspace.red[out.curx] += RedVal( c );
									workspace.blue[out.curx] += BlueVal( c );
									workspace.green[out.curx] += GreenVal( c );
									workspace.alpha[out.curx] += AlphaVal( c );
									workspace.samples[out.curx]++;
								}
								//*IMG_ADDRESS(dest,out.curx,out.cury) = DOALPHA( *po, c, AlphaVal(c) ) ;
							}
#endif
							//plot( dest, out.curx, out.cury
							//	 , getpixel( source, source.image.curx >> SCALE_SHIFT, source.image.cury >> SCALE_SHIFT )
								  //, Color( source.image.curx*4, source.image.cury*4, 0 )
							//	 );
							out.err.x += out.del.image.x;
							out.curx++;
							while( out.curx <= right.curx && out.err.x > 0 )
							{
								out.err.x -= out.del.out.x;
								{ // iterate the image one step. (ie out.image++ )
									source.image.err += source.image.del.x;
									source.image.cury += source.image.incy;
								if( ( ( source.image.curx >> SCALE_SHIFT ) < source_image->width ) &&
								  ( ( source.image.cury >> SCALE_SHIFT ) < source_image->height ) )
									{
										CDATA c = *IMG_ADDRESS( source_image, source.image.curx >> SCALE_SHIFT, source.image.cury >> SCALE_SHIFT );
										workspace.red[out.curx] += RedVal( c );
										workspace.blue[out.curx] += BlueVal( c );
										workspace.green[out.curx] += GreenVal( c );
										workspace.alpha[out.curx] += AlphaVal( c );
										workspace.samples[out.curx]++;
									}
									while( source.image.err > 0 )
									{
										source.image.err -= source.image.del.y;
										source.image.curx += source.image.incx;
								if( ( ( source.image.curx >> SCALE_SHIFT ) < source_image->width ) &&
								  ( ( source.image.cury >> SCALE_SHIFT ) < source_image->height ) )
										{
											if( out.curx > 0 )
											{
												CDATA c = *IMG_ADDRESS( source_image, source.image.curx >> SCALE_SHIFT, source.image.cury >> SCALE_SHIFT );
												workspace.red[out.curx] += RedVal( c );
												workspace.blue[out.curx] += BlueVal( c );
												workspace.green[out.curx] += GreenVal( c );
												workspace.alpha[out.curx] += AlphaVal( c );
												workspace.samples[out.curx]++;
											}
										}
									}
								}
							}
						}
						// output the workspace.
						{
							for( out.curx = left.curx; out.curx < right.curx; out.curx++)
							{
								if( workspace.samples[out.curx] )
								{
									CDATA *po = IMG_ADDRESS(dest,out.curx,out.cury);
									CDATA c = AColor( CONVERT_SAMPLE( red, out.curx )
														 , CONVERT_SAMPLE( blue, out.curx )
														 , CONVERT_SAMPLE( green, out.curx )
														 , CONVERT_SAMPLE( alpha, out.curx )
														 );
									*IMG_ADDRESS(dest,out.curx,out.cury) = DOALPHA( *po, c, AlphaVal(c) ) ;
								}
							}
						}

					}
				}
				else
				{
					out.del.image.x = source.image.del.x;
					out.del.image.y = source.image.del.y;
					{
						out.err.x = (-out.del.image.x)/2;
						source.image.err = (-source.image.del.x)/2;
						//lprintf( "plot..." );
						cols = 0;
						while( out.curx <= right.curx )
						{
							cols++;
							//printf( "c %d,", source.image.curx );
#ifdef OUTPUT_IMAGE
							if( ( out.curx >= dest->x ) && ( out.curx < (dest->x + dest->width) )
								&& ( out.cury >= dest->y ) && ( out.cury < (dest->y + dest->height) ) )
							{
								if( ( source.image.curx >> SCALE_SHIFT < source_image->width ) &&
								  ( source.image.cury >> SCALE_SHIFT < source_image->height ) )
								if( out.curx > 0 )
								{
									CDATA c = *IMG_ADDRESS( source_image, source.image.curx >> SCALE_SHIFT, source.image.cury >> SCALE_SHIFT );
									//CDATA *po = IMG_ADDRESS(dest,out.curx,out.cury);
									workspace.red[out.curx] += RedVal( c );
									workspace.blue[out.curx] += BlueVal( c );
									workspace.green[out.curx] += GreenVal( c );
									workspace.alpha[out.curx] += AlphaVal( c );
									workspace.samples[out.curx]++;
								}
								//CDATA c = *IMG_ADDRESS( source_image, source.image.curx >> SCALE_SHIFT, source.image.cury >> SCALE_SHIFT );
								//CDATA *po = IMG_ADDRESS(dest,out.curx,out.cury);
								//*IMG_ADDRESS(dest,out.curx,out.cury) = DOALPHA( *po, c, AlphaVal(c) ) ;
							}
#endif
							//plot( dest, out.curx, out.cury
							//	 , getpixel( source, source.image.curx >> SCALE_SHIFT, source.image.cury >> SCALE_SHIFT )
								  //, Color( source.image.curx*4, source.image.cury*4, 0 )
							//	 );
							out.err.x += out.del.image.x;
							out.curx++;
							while( out.curx <= right.curx && out.err.x > 0 )
							{
								out.err.x -= out.del.out.x;
								{ // iterate the image one step. (ie out.image++ )
									source.image.err += source.image.del.y;
									source.image.curx += source.image.incx;
									{
								if( ( source.image.curx>>SCALE_SHIFT < source_image->width ) &&
									( source.image.cury>>SCALE_SHIFT < source_image->height ) )
								if( out.curx > 0 )
								{
										CDATA c = *IMG_ADDRESS( source_image, source.image.curx >> SCALE_SHIFT, source.image.cury >> SCALE_SHIFT );
										workspace.red[out.curx] += RedVal( c );
										workspace.blue[out.curx] += BlueVal( c );
										workspace.green[out.curx] += GreenVal( c );
										workspace.alpha[out.curx] += AlphaVal( c );
										workspace.samples[out.curx]++;
								}
									}
									while( source.image.err > 0 )
									{
										source.image.err -= source.image.del.x;
										source.image.cury += source.image.incy;
										{
								if( ( source.image.curx>>SCALE_SHIFT < source_image->width ) &&
									( source.image.cury>>SCALE_SHIFT < source_image->height ) )
								if( out.curx > 0 )
								{
											CDATA c = *IMG_ADDRESS( source_image, source.image.curx >> SCALE_SHIFT, source.image.cury >> SCALE_SHIFT );
											//CDATA *po = IMG_ADDRESS(dest,out.curx,out.cury);
											workspace.red[out.curx] += RedVal( c );
											workspace.blue[out.curx] += BlueVal( c );
											workspace.green[out.curx] += GreenVal( c );
											workspace.alpha[out.curx] += AlphaVal( c );
											workspace.samples[out.curx]++;
								}
										}
									}
								}
							}
						}
                  // output workspace.
						{
							for( out.curx = left.curx; out.curx < right.curx; out.curx++)
							{
								if( out.curx >= dest->x &&
									out.cury >= dest->y &*&
									out.curx < (dest->x+dest->width) &&
									out.cury < (dest->y+dest->height ) )
								if( workspace.samples[out.curx] )
								{
									CDATA *po = IMG_ADDRESS(dest,out.curx,out.cury);
									CDATA c = AColor( CONVERT_SAMPLE( red, out.curx )
														 , CONVERT_SAMPLE( blue, out.curx )
														 , CONVERT_SAMPLE( green, out.curx )
														 , CONVERT_SAMPLE( alpha, out.curx )
														 );
									*IMG_ADDRESS(dest,out.curx,out.cury) = DOALPHA( *po, c, AlphaVal(c) ) ;
								}
							}
						}
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
						left.err2 -= left.del.out; // the larger distance to go on the line.
						left.image.err += left.image.del.x;
						// increment the longer distance by a unit increment.
						left.image.cury += left.image.incy;
						while( left.image.err > 0 )
						{
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
						left.err2 -= left.del.out; // the larger distance to go on the line.
						left.image.err += left.image.del.y;
						// incement the longer distance by a unit distance.
						left.image.curx += left.image.incx;
                  //DebugBreak();
						//printf( "." );
						while( left.image.err > 0 )
						{
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
					right.err -= right.del.y;
					right.curx += right.incx;
				}

				right.err2 += right.del.image;
				if( right.image.del.x < right.image.del.y )
				{
					while( right.err2 > 0 )
					{
						right.err2 -= right.del.out; // the larger distance to go on the line.
						right.image.err += right.image.del.x;
						// increment the longer distance by a unit increment.
						right.image.cury += right.image.incy;
						while( right.image.err > 0 )
						{
							right.image.err -= right.image.del.y;
							right.image.curx += right.image.incx;
						}
					}
				}
				else // right.image.del.x < right.image.del.y
				{
					while( right.err2 > 0 )
					{
						right.err2 -= right.del.out; // the larger distance to go on the line.
						right.image.err += right.image.del.y;
						// increment the longer distance by a unit increment.
						right.image.curx += right.image.incx;
						while( right.image.err > 0 )
						{
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
		lprintf( "lines : %" _32f " cols: %" _32f "", lines, cols );
#endif
	}
   used = 0;
}

static void (CPROC *SavePortion )( PSPRITE_METHOD psm, _32 x, _32 y, _32 w, _32 h );


IMAGE_PROC( void, SetSavePortion )( void (CPROC*_SavePortion )( PSPRITE_METHOD psm, _32 x, _32 y, _32 w, _32 h ) )
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
	static _32 lock;
	static PTRANSFORM transform;
	//lprintf( "-- Begin Transform" );
	while( LockedExchange( &lock, 1 ) ) Relinquish();
	if( !transform )
		transform = CreateNamedTransform( NULL );
	Translate( transform
				, (RCOORD)sprite->curx * sprite->scalex / (RCOORD)0x10000
				, (RCOORD)sprite->cury*sprite->scaley / (RCOORD)0x10000
				, (RCOORD)0 );
	//lprintf( WIDE("angle = %ld"), sprite->angle );
	Scale( transform, sprite->scalex / (RCOORD)0x10000, sprite->scaley / (RCOORD)0x10000, 0 );
	RotateAbs( transform, (RCOORD)0, (RCOORD)0, (RCOORD)sprite->angle );
	//Scale( transform, 1, 1, 0 );
	tmp[0] = (0) - sprite->hotx;
	tmp[1] = (0) - sprite->hoty;
	tmp[2] = 0;
	Apply( transform, result, tmp );
	sprite->minx = sprite->maxx = x1 = result[0]; // + sprite->curx;
	sprite->miny = sprite->maxy = y1 = result[1]; // + sprite->cury;
	tmp[0] = (sprite->image->real_width) - sprite->hotx;
	tmp[1] = (0) - sprite->hoty;
	tmp[2] = 0;
	Apply( transform, result, tmp );
	x2 = result[0]; // + sprite->curx;
	if( x2 > sprite->maxx ) sprite->maxx = x2;
	if( x2 < sprite->minx ) sprite->minx = x2;
	y2 = result[1]; // + sprite->cury;
	if( y2 > sprite->maxy ) sprite->maxy = y2;
	if( y2 < sprite->miny ) sprite->miny = y2;
	tmp[0] = (sprite->image->real_width) - sprite->hotx;
	tmp[1] = (sprite->image->real_height) - sprite->hoty;
	tmp[2] = 0;
	Apply( transform, result, tmp );
	x3 = result[0]; // + sprite->curx;
	if( x3 > sprite->maxx ) sprite->maxx = x3;
	if( x3 < sprite->minx ) sprite->minx = x3;
	y3 = result[1]; // + sprite->cury;
	if( y3 > sprite->maxy ) sprite->maxy = y3;
	if( y3 < sprite->miny ) sprite->miny = y3;
	tmp[0] = (0) - sprite->hotx;
	tmp[1] = (sprite->image->real_height) - sprite->hoty;
	tmp[2] = 0;
	Apply( transform, result, tmp );
	x4 = result[0]; // + sprite->curx;
	if( x4 > sprite->maxx ) sprite->maxx = x4;
	if( x4 < sprite->minx ) sprite->minx = x4;
	y4 = result[1]; // + sprite->cury;
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
			lprintf( "save portion? ");
#endif
			SavePortion( sprite->pSpriteMethod
				, sprite->minx, sprite->miny
				, (sprite->maxx - sprite->minx) + 1
				, (sprite->maxy - sprite->miny) + 1 );
#ifdef DEBUG_TIMING
			lprintf( "saved.." );
#endif
		}
	}
#ifdef DEBUG_TIMING
	lprintf( "Output arbitrary" );
#endif
	PlotArbitrary( dest, sprite->image
		, x1, y1
		, x2, y2
		, x3, y3
		, x4, y4
		, 0
		, BLOT_COPY
		, 0, 0, 0 );
#ifdef DEBUG_TIMING
	lprintf( "arbitrary out" );
#endif
}





/* rotate_scaled_sprite:
 *  Draws a sprite image onto a bitmap at the specified position, rotating 
 *  it by the specified angle. The angle is a fixed point 16.16 number in 
 *  the same format used by the fixed point trig routines, with 256 equal 
 *  to a full circle, 64 a right angle, etc. This function can draw onto
 *  both linear and mode-X bitmaps.
 */
IMAGE_PROC(  void, rotate_scaled_sprite )(ImageFile *bmp, SPRITE *sprite, fixed angle, fixed scale_width, fixed scale_height)
{
	//lprintf( "REnder sprite..." );
	//lprintf( "input angle = %ld", angle );
	sprite->angle = ( ( 2 * 3.14159268 ) * angle ) / 0x100000000LL;
	//lprintf( "output angle si %g", sprite->angle );
	sprite->scalex = scale_width;
	sprite->scaley = scale_height;
	TranslatePoints( bmp, sprite );
	lprintf( "Done render (SMOOTH)sprite..." );
}

/* rotate_sprite:
 *  Draws a sprite image onto a bitmap at the specified position, rotating 
 *  it by the specified angle. The angle is a fixed point 16.16 number in 
 *  the same format used by the fixed point trig routines, with 256 equal 
 *  to a full circle, 64 a right angle, etc. This function can draw onto
 *  both linear and mode-X bitmaps.
 */
IMAGE_PROC(  void, rotate_sprite )(ImageFile *bmp, SPRITE *sprite, fixed angle)
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
