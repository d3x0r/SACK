#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii


#include <controls.h>
#include "land.h"
#include "local.h"


extern struct world_body_map bodymap;
extern int cur_x, cur_y, cur_s;


#define PATCH_PAD 5

#define PATCH_LEFT(n) ( ( (n) * surface->width - ( 5 * PATCH_PAD ) ) / 4 )
#define PATCH_WIDTH(n) (PATCH_LEFT(n+1)-PATCH_LEFT(n))
#define PATCH_RIGHT(n) ( ( (n) * surface->height - ( 5 * PATCH_PAD ) ) / 4 )
#define PATCH_CELL_PAD 1
#define PATCH_CELL_SIZE ( (PATCH_WIDTH(p_c)-(PATCH_CELL_PAD*HEX_SIZE))/HEX_SIZE)

#define patch_left(n)  (( (n) * (HEX_SIZE*PATCH_CELL_SIZE + PATCH_PAD )) + PATCH_PAD)  
#define patch_width(n) (patch_left(n+1)-patch_left(n)-(PATCH_PAD))


#define patch_top(n)  (( (n) * (HEX_SIZE*PATCH_CELL_SIZE + PATCH_PAD )) + PATCH_PAD)  
#define patch_height(n) (patch_top(n+1)-patch_top(n)-(PATCH_PAD))

//#define PATCH_SIZE patch_width( p_c )



#define r_left(n)   ((patch_left(p_c) + \
					( ( (n)  * ( patch_width(p_c)-PATCH_CELL_PAD))/ HEX_SIZE ) ) + PATCH_CELL_PAD)

#define r_width(n)   r_left(n+1)-r_left(n) - (PATCH_CELL_PAD)

#define r_top(n)   ((patch_top(p_r) + \
					( ( (n)  * ( patch_height(p_r) -(PATCH_CELL_PAD)))/ HEX_SIZE ) ) + PATCH_CELL_PAD)

#define r_height(n)   r_top(n+1)-r_top(n)-(PATCH_CELL_PAD)


int CPROC DrawGrid( PSI_CONTROL frame )
	{
		int p_r, p_c, p, x, y;
      Image surface = GetControlSurface( frame );
      ClearImageTo( surface, BASE_COLOR_BLACK );
		for( p_r = 0; p_r < 3; p_r++ )
		{
			for( p_c = 0; p_c < 4; p_c++ )
			{
				p = p_r*4 + p_c;
				BlatColor( surface
							, patch_left(p_c)
							, patch_top( p_r )
							, patch_width(p_c)
							, patch_height( p_r )
							, BASE_COLOR_DARKGREY
							);
				for( x = 0; x < HEX_SIZE; x++ )
					for( y = 0; y < HEX_SIZE; y++ )
					{
						// 640 / 4 = 160
						// 480 / 3 = 160
						//lprintf( WIDE("r_left is %d -> %d r_right is %d"), r_left(x), r_left(x+1), r_width(x) );

						if( x == cur_x && y == cur_y && p == cur_s )
						{
							if( bodymap.band[p][x][y] )
							{
								BlatColor( surface, r_left(x), r_top( y )
											, r_width(x), r_height( y )
											, BASE_COLOR_YELLOW
											);
							}
							else
							{
								BlatColor( surface, r_left(x), r_top( y )
											, r_width(x), r_height( y )
											, BASE_COLOR_RED
											);
							}
						}
						else
						{
							if( bodymap.band[p][x][y] )
							{
								BlatColor( surface
											, r_left(x), r_top( y )
											, r_width(x), r_height( y )
											, BASE_COLOR_BLUE
											);
							}
							else
							{
								BlatColor( surface
											, r_left(x), r_top( y )
											, r_width(x), r_height( y )
											, BASE_COLOR_GREEN
											);
							}
						}
					}
			}
		}
		return 1;
	}

void CPROC UpdateGrid( uintptr_t psv )
{
	SmudgeCommon( (PSI_CONTROL)psv );
}

void CreateGPSGridDisplay( void )
{
	PSI_CONTROL frame;
	frame = CreateFrame( WIDE("GPS Grid Display"), 640, 480, 640, 480, BORDER_RESIZABLE|BORDER_NORMAL, NULL );

	AddCommonDraw( frame, DrawGrid) ;

	AddTimer( 300, UpdateGrid, (uintptr_t)frame );
	DisplayFrame( frame );
	return;
}



