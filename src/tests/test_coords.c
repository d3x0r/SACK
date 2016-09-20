#include <stdhdrs.h>

typedef struct fake_page
{
	struct {
      int nPartsX, nPartsY;
	} grid;
} PAGE, *PPAGE;

typedef struct fake_canvas
{
   PAGE page;
   PPAGE current_page;
	uint32_t width, height;
   uint32_t nPartsX, nPartsY;
} CANVAS, *PCANVAS;

#define PART_RESOLUTION 4096
#define _X(canvas,n) (((n) * (canvas)->width)/(PART_RESOLUTION))
#define _Y(canvas,n) (((n) * (canvas)->height)/(PART_RESOLUTION))

#define _WIDTH(canvas,w) ( ( (PART_RESOLUTION) - ( (canvas)->button_space * ((canvas)->button_cols + 1) ) ) / (canvas)->button_cols );

#define _COMPUTEPARTOFX( canvas,x, parts )  ((x)*parts / ((canvas)->width) )
#define _COMPUTEPARTOFY( canvas,y, parts )  ((y)*parts / ((canvas)->height) )

#define _COMPUTEX( canvas,npart, parts )  ( ( ( ( (PART_RESOLUTION) * (npart) ) ) * ((canvas)->width) ) / ((parts)*PART_RESOLUTION) )
#define _COMPUTEY( canvas,npart, parts )  ( ( ( ( (PART_RESOLUTION) * (npart) ) ) * ((canvas)->height) ) / ((parts)*PART_RESOLUTION) )

#define _MODX( canvas,npart, parts )  ( ( ( ( (PART_RESOLUTION) * npart ) ) * ((canvas)->width) ) % ((parts)*PART_RESOLUTION) )
#define _MODY( canvas,npart, parts )  ( ( ( ( (PART_RESOLUTION) * npart ) ) * ((canvas)->height) ) % ((parts)*PART_RESOLUTION) )

#define _PARTX(canvas,part) (int32_t)_COMPUTEX(canvas,part,(canvas)->current_page->grid.nPartsX)
#define _PARTY(canvas,part) (int32_t)_COMPUTEY(canvas,part,(canvas)->current_page->grid.nPartsY)

#define _PARTW(canvas,x,w) (uint32_t)(_PARTX(canvas,x+w)-_PARTX(canvas,x))
#define _PARTH(canvas,y,h) (uint32_t)(_PARTY(canvas,y+h)-_PARTY(canvas,y))
#define _PARTSX(canvas) (canvas)->current_page->grid.nPartsX
#define _PARTSY(canvas) (canvas)->current_page->grid.nPartsY

#define X(n) _X(canvas)
#define Y(n) _Y(canvas)

#define WIDTH(w) _WIDTH(canvas,w)

#define COMPUTEPARTOFX(x,parts)        _COMPUTEPARTOFX(canvas,x,parts )
#define COMPUTEPARTOFY(y,parts)        _COMPUTEPARTOFY(canvas,y,parts )
#define COMPUTEX( npart, parts ) _COMPUTEX( canvas,npart, parts )
#define COMPUTEY( npart, parts ) _COMPUTEY( canvas,npart, parts )
#define MODX( npart, parts )     _MODX( canvas,npart, parts )
#define MODY( npart, parts )     _MODY( canvas,npart, parts )
#define PARTX(part)              _PARTX(canvas,part)
#define PARTY(part)              _PARTY(canvas,part)
#define PARTW(x,w)               _PARTW(canvas,x,w)
#define PARTH(y,h)               _PARTH(canvas,y,h)
#define PARTSX                   _PARTSX(canvas)
#define PARTSY                   _PARTSY(canvas)


#define PARTOFX(xc) ( ( xc ) * canvas->current_page->grid.nPartsX ) / canvas->width
#define PARTOFY(yc) ( ( yc ) * canvas->current_page->grid.nPartsY ) / canvas->height



int main( void )
{
	CANVAS tmp;
	PCANVAS canvas = &tmp;
   tmp.current_page = &tmp.page;
	tmp.width = 1920;
	tmp.height =1080;
	tmp.page.grid.nPartsX=	tmp.nPartsX = 96;
	tmp.page.grid.nPartsY=	tmp.nPartsY = 52;
	{
		int n;
		for( n = 0; n < 1920; n++ )
		{
         int peice = PARTOFX( n );
			int part = PARTX( peice );
			int real = COMPUTEPARTOFX( part, tmp.nPartsX );

         int peice2 = ( ( n ) * (canvas->current_page->grid.nPartsX/2) ) / canvas->width;
			int part2 = _COMPUTEX(canvas,peice2,(canvas)->current_page->grid.nPartsX/2);
			int real2 = COMPUTEPARTOFX( part, tmp.nPartsX/2 );
			lprintf( "------ \nPart is %d : %d for %d", peice, part, n );
			lprintf( "and %d for %d", real, part );

			lprintf( "======\n Part is %d : %d for %d", peice2, part2, n );
			lprintf( "and %d for %d", real2, part2 );

		}
	}
   return 0;
}

