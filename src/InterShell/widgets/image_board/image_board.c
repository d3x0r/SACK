
#include "image_board.h"

EasyRegisterControlWithBorderEx( "ImageBoard", sizeof( struct image_board ), BORDER_NONE, ImageBoardControl );
EasyRegisterControlWithBorderEx( "ImageBoard Cell", sizeof( struct image_cell ), BORDER_NONE, ImageBoardCellControl );


static int OnCreateCommon( "ImageBoard" )( PSI_CONTROL pc )
{

   return 1;
}

static uintptr_t OnCreateControl( "ImageBoard" )( PSI_CONTROL parent, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
   return MakeNamedControl( parent, "ImageBoard", x, y, w, h, -1 );
}

static PSI_CONTROL OnQueryGetControl( "ImageBoard" )( uintptr_t psv )
{
   return (PSI_CONTROL)psv;
}

static void OnLoadControl(WIDE("ImageBoard"))( PCONFIG_HANDLER psv, uintptr_t psv )
{
	PSI_CONTROL pc = (PSI_CONTROL)psv;
	ValidatedControlData( struct image_board *, ImageBoardControl.TypeID, board, pc );

   AddConfigurationMethod(
}

static void OnSaveControl( WIDE("ImageBoard" ))( FILE *file, uintptr_t psv )
{

}
