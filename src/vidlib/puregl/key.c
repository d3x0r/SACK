#include <stdhdrs.h>
//#include <windows.h>
#include <vectlib.h>

#ifdef __ANDROID__
#include <GLES2/gl2.h>
#else
#include <GL/gl.h>
#endif
#include <render.h>
#include <vidlib/vidstruc.h>
//#define PRENDERER PVIDEO
//#include <vidlib.h>
#include <render.h>
#include "keybrd.h"  // includes vidlib

RENDER_NAMESPACE

#include "local.h"

//------------------------------------------------------------------------

RENDER_PROC( _32, IsKeyDown )( PRENDERER hVideo, int c )
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

RENDER_PROC( _32, KeyDown )( PRENDERER hVideo, int c )
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

RENDER_PROC( int, KeyUp )(  PRENDERER hVideo, int c )
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

RENDER_PROC( LOGICAL, KeyDouble )( PRENDERER hVideo, int c )
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

//------------------------------------------------------------------------

// $Log: key.c,v $
// Revision 1.12  2005/03/14 11:07:00  panther
// Link all windows above something... so all windows of an application promote in application order.
//
// Revision 1.11  2004/09/22 20:25:20  d3x0r
// Begin implementation of message queues to handle events from video to application
//
// Revision 1.10  2004/06/21 07:46:45  d3x0r
// Account for newly moved structure files.
//
// Revision 1.9  2004/04/27 09:55:11  d3x0r
// Add keydef to keyhandler path
//
// Revision 1.8  2003/11/22 23:40:39  panther
// Fix includes so there's no conflict with winuser.h
//
// Revision 1.7  2003/07/24 11:58:36  panther
// Dont' define prenderer - makes watcom unhappy
//
// Revision 1.6  2003/03/25 08:45:58  panther
// Added CVS logging tag
//
