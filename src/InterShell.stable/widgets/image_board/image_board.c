
#include "image_board.h"

EasyRegisterControlWithBorderEx( "ImageBoard", sizeof( struct image_board ), BORDER_NONE, ImageBoardControl );
EasyRegisterControlWithBorderEx( "ImageBoard Cell", sizeof( struct image_cell ), BORDER_NONE, ImageBoardCellControl );


static int OnCreateCommon( "ImageBoard" )( PSI_CONTROL pc )
{

   return 1;
}

static PTRSZVAL OnCreateControl( "ImageBoard" )( PSI_CONTROL parent, S_32 x, S_32 y, _32 w, _32 h )
{
   return MakeNamedControl( parent, "ImageBoard", x, y, w, h, -1 );
}

static PSI_CONTROL OnQueryGetControl( "ImageBoard" )( PTRSZVAL psv )
{
   return (PSI_CONTROL)psv;
}

static void OnLoadControl(WIDE("ImageBoard"))( PCONFIG_HANDLER psv, PTRSZVAL psv )
{
	PSI_CONTROL pc = (PSI_CONTROL)psv;
	ValidatedControlData( struct image_board *, ImageBoardControl.TypeID, board, pc );

   AddConfigurationMethod(
}

static void OnSaveControl( WIDE("ImageBoard" ))( FILE *file, PTRSZVAL psv )
{

}
