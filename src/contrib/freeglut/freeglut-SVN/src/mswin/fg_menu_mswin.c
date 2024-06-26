/*
 * freeglut_menu_mswin.c
 *
 * The Windows-specific mouse cursor related stuff.
 *
 * Copyright (c) 2012 Stephen J. Baker. All Rights Reserved.
 * Written by John F. Fay, <fayjf@sourceforge.net>
 * Creation date: Sun Jan 22, 2012
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PAWEL W. OLSZTA BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define FREEGLUT_BUILDING_LIB
#include <GL/freeglut.h>
#include "../fg_internal.h"

extern void fghGetClientArea( RECT *clientRect, const SFG_Window *window );
extern SFG_Window* fghWindowUnderCursor(SFG_Window *window);


GLvoid fgPlatformGetGameModeVMaxExtent( SFG_Window* window, int* x, int* y )
{
    *x = glutGet ( GLUT_SCREEN_WIDTH );
    *y = glutGet ( GLUT_SCREEN_HEIGHT );
}

void fgPlatformCheckMenuDeactivate()
{
    /* If we have an open menu, see if the open menu should be closed
     * when focus was lost because user either switched
     * application or FreeGLUT window (if one is running multiple
     * windows). If so, close menu the active menu.
     */
    SFG_Menu* menu = NULL;

    if ( fgStructure.Menus.First )
        menu = fgGetActiveMenu();

    if ( menu )
    {
        SFG_Window* wnd = NULL;
        HWND hwnd = GetFocus();  /* Get window with current focus - NULL for non freeglut windows */
        if (hwnd)
            /* See which of our windows it is */
            wnd = fgWindowByHandle(hwnd);

        if (!hwnd || !wnd)
            /* User switched to another application*/
            fgDeactivateMenu(menu->ParentWindow);
        else if (!wnd->IsMenu)      /* Make sure we don't kill the menu when trying to enter a submenu */
        {
            /* we need to know if user clicked a child window, any displayable area clicked that is not the menu's parent window should close the menu */
            wnd = fghWindowUnderCursor(wnd);
            if (wnd!=menu->ParentWindow)
                /* User switched to another FreeGLUT window */
                fgDeactivateMenu(menu->ParentWindow);
            else
            {
                /* Check if focus lost because non-client area of
                 * window was pressed (pressing on client area is
                 * handled in fgCheckActiveMenu)
                 */
                POINT mouse_pos;
                RECT clientArea;
                fghGetClientArea(&clientArea,menu->ParentWindow);
                GetCursorPos(&mouse_pos);
                if ( !PtInRect( &clientArea, mouse_pos ) )
                    fgDeactivateMenu(menu->ParentWindow);
            }
        }
    }
};



/* -- PLATFORM-SPECIFIC INTERFACE FUNCTION -------------------------------------------------- */

int FGAPIENTRY __glutCreateMenuWithExit( void(* callback)( int ), void (__cdecl *exit_function)(int) )
{
  __glutExitFunc = exit_function;
  return glutCreateMenu( callback );
}

