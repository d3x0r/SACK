#include <psi.h>


struct image_attrib
{
	Image image;
   CDATA color;
};


struct image_cell
{
	struct image_attrib background;
	Image cell;
   struct image_board *container;
};

struct image_board
{
	struct image_attrib background;
   Image cell_original;
	PLIST cells;
   int rows, cols;
};



