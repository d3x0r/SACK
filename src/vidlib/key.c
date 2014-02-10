#include <stdhdrs.h>

#include <vidlib/vidstruc.h>
#include <render.h>
#include "keybrd.h"  // includes vidlib


#include "local.h"

RENDER_NAMESPACE
//------------------------------------------------------------------------

RENDER_PROC( _32, IsKeyDown )( PVIDEO hVideo, int c )
{
   if( hVideo )
		return hVideo->kbd.key[c] & 0x80;
	else
	{
      //lprintf( WIDE("test local key %d as %d vs %d"), c, l.kbd.key[c], 0x80 );
		return l.kbd.key[c] & 0x80;
	}
}

//------------------------------------------------------------------------

RENDER_PROC( _32, KeyDown )( PVIDEO hVideo, int c )
{
	if( hVideo )
	{
		if( hVideo->kbd.key[c] & 0x80 && (hVideo->kbd.keyupdown[c] & KEYISDOWN) ) // don't repress...
		{
			return FALSE;
		}
		else
		{
			if( hVideo->kbd.key[c] & 0x80 )  // if key IS hit
			{
				hVideo->kbd.keyupdown[c] |= KEYISDOWN;  // keydown was not also true...
				if( ( hVideo->kbd.keytime[c] + 250 ) <= timeGetTime() )
					hVideo->kbd.keydouble[c] = TRUE;
				hVideo->kbd.keytime[c] = timeGetTime();
			}
			else
			{
				hVideo->kbd.keyupdown[c] &= ~KEYISDOWN; // key is NOT down at all...
			}
			return hVideo->kbd.keyupdown[c] & KEYISDOWN; // return edge triggered state...
		}
	}
	else
	{
		if( l.kbd.key[c] & 0x80 && (l.kbd.keyupdown[c] & KEYISDOWN) ) // don't repress...
		{
			return FALSE;
		}
		else
		{
			if( l.kbd.key[c] & 0x80 )  // if key IS hit
			{
				l.kbd.keyupdown[c] |= KEYISDOWN;  // keydown was not also true...
				if( ( l.kbd.keytime[c] + 250 ) <= timeGetTime() )
					l.kbd.keydouble[c] = TRUE;
				l.kbd.keytime[c] = timeGetTime();
			}
			else
			{
				l.kbd.keyupdown[c] &= ~KEYISDOWN; // key is NOT down at all...
			}
			return l.kbd.keyupdown[c] & KEYISDOWN; // return edge triggered state...
		}
	}

}

//------------------------------------------------------------------------

RENDER_PROC( int, KeyUp )(  PVIDEO hVideo, int c )
{
	if( hVideo )
	{
		if( !(hVideo->kbd.key[c] & 0x80) && (hVideo->kbd.keyupdown[c] & KEYISUP) ) // don't repress...
		{
			return FALSE;
		}
		else
		{
			if( !(hVideo->kbd.key[c] & 0x80) )  // if key IS hit
			{
				hVideo->kbd.keyupdown[c] |= KEYISUP;  // keydown was not also true...
				if( ( hVideo->kbd.keytime[c] + 250 ) <= timeGetTime() )
					hVideo->kbd.keydouble[c] = TRUE;
				hVideo->kbd.keytime[c] = timeGetTime();
			}
			else
			{
				hVideo->kbd.keyupdown[c] &= ~KEYISUP; // key is NOT up at all...
			}
			return hVideo->kbd.keyupdown[c] & KEYISUP; // return edge triggered state...
		}
	}
	else
	{
		if( !(l.kbd.key[c] & 0x80) && (l.kbd.keyupdown[c] & KEYISUP) ) // don't repress...
		{
			return FALSE;
		}
		else
		{
			if( !(l.kbd.key[c] & 0x80) )  // if key IS hit
			{
				l.kbd.keyupdown[c] |= KEYISUP;  // keydown was not also true...
				if( ( l.kbd.keytime[c] + 250 ) <= timeGetTime() )
					l.kbd.keydouble[c] = TRUE;
				l.kbd.keytime[c] = timeGetTime();
			}
			else
			{
				l.kbd.keyupdown[c] &= ~KEYISUP; // key is NOT up at all...
			}
			return l.kbd.keyupdown[c] & KEYISUP; // return edge triggered state...
		}
	}

}

//------------------------------------------------------------------------

RENDER_PROC( BOOL, KeyDouble )( PVIDEO hVideo, int c )
{
	if( hVideo )
	{
		if( KeyDown( hVideo, c) )
			if( hVideo->kbd.keydouble[c] )
			{
				hVideo->kbd.keydouble[c] = FALSE;  // clear keydouble....... incomplete...
				return TRUE;
			}
	}
	else
	{
		if( KeyDown( hVideo, c) )
			if( l.kbd.keydouble[c] )
			{
				l.kbd.keydouble[c] = FALSE;  // clear keydouble....... incomplete...
				return TRUE;
			}
	}
   return FALSE;
}
RENDER_NAMESPACE_END
