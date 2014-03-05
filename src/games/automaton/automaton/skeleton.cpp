

#include "editor.hpp"


void EDITOR::EditMouseCallback( int x, int y, int b )
{
   static int _x, _y, _right, _left;
   int right, left;
   right = b & MK_RBUTTON;
   left = b & MK_LBUTTON;




   _x = x;
   _y = y;
   _right = right;
   _left  = left;
}

void EDITOR::EditResizeCallback( void )
{
}
   
void EDITOR::EditCloseCallback( void )
{
}

static void EditMouseCallback( DWORD dwUser, int x, int y, int b )
{
   class EDITOR *Editor = (class EDITOR*)dwUser;
   Editor->EditMouseCallback( x, y, b );
}

static void EditResizeCallback( DWORD dwUser )
{
   class EDITOR *Editor = (class EDITOR*)dwUser;
   Editor->EditResizeCallback( );
}

static void EditWindowClose( DWORD dwUser )
{
   class EDITOR *Editor = (class EDITOR*)dwUser;
   Editor->EditWindowClose(  );
}


EDITOR( void )
{
   hVideo = InitVideo( TRUE, "World Editor" );
   hVideo->pMouseCallback  = EditMouseCallback;
   hVideo->pResizeCallback = EditResizeCallback;
   hVideo->pWindowClose    = EditWindowClose;
   hVideo->dwUser = (DWORD)this;
}

~EDITOR( void )
{
   FreeVideo( hVideo );
}