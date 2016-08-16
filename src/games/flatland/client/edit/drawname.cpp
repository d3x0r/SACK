#include <stdhdrs.h>

#include "flatland_global.h"

#include <image.h>
#include <world.h>

extern PIMAGE_INTERFACE MyImageInterface;
// x and y is center point.
void DrawName( Image pImage, PNAME pName, int x, int y )
{
	char SectorName[64];
	uint32_t w, h, l;
	GetStringSize( NULL, NULL, &h );
	if( pName->flags.bVertical )
	{
		x += ( h * (pName->lines - 2) ) / 2;
		for( l = 0; l < pName->lines; l++ )
		{
			GetStringSizeEx( pName->name[l].name, pName->name[l].length, &w, NULL );
			PutStringVerticalEx( pImage
						, x 
						, y - w/2
						, AColor( 255, 255, 255, 160 )
						, 0
						, pName->name[l].name, pName->name[l].length );
			x -= h;
		}
	}
	else
	{
		y -= ( h * pName->lines ) / 2;
		for( l = 0; l < pName->lines; l++ )
		{
			GetStringSizeEx( pName->name[l].name, pName->name[l].length, &w, NULL );
			PutStringEx( pImage
						, x - w/2
						, y
						, AColor( 255, 255, 255, 160 )
						, 0
						, pName->name[l].name, pName->name[l].length );
			y += h;
		}
	}
}
