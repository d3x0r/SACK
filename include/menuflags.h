
// copied from win.h distributed with lcc
//#define MF_BITMAP	4
//#define MF_CHECKED	8
//#define MF_DISABLED	2
//#define MF_ENABLED	0
//#define MF_GRAYED	1
//#define MF_MENUBARBREAK	32
//#define MF_MENUBREAK	64
//#define MF_OWNERDRAW	256
//#define MF_POPUP	16
//#define MF_SEPARATOR	0x800
//#define MF_STRING	0
//#define MF_UNCHECKED	0
//#define MF_DEFAULT	4096
//#define MF_SYSMENU	0x2000
//#define MF_HELP		0x4000
//#define MF_END	128
//#define MF_RIGHTJUSTIFY 0x4000
//#define MF_MOUSESELECT 0x8000

// duplicated from a section futher down...
//#define MF_BYCOMMAND	0
//#define MF_BYPOSITION	0x400
//#define MF_UNCHECKED	0
//#define MF_HILITE	128
//#define MF_UNHILITE	0


#ifndef MF_BYPOSITION
#define MF_BYPOSITION 0x400
#endif
#ifndef MF_BYCOMMAND
#define MF_BYCOMMAND 0
#endif

#ifndef MF_HILITE
#define MF_HILITE 0x80
#endif
#ifndef MF_UNHILITE
#define MF_UNHILITE 0
#endif

#ifndef MF_BITMAP
#define MF_BITMAP 4
#endif
#ifndef MF_CHECKED
#define MF_CHECKED 8
#endif
#ifndef MF_DISABLED
#define MF_DISABLED 0x0010
#endif
#ifndef MF_ENABLED
#define MF_ENABLED 0
#endif
#ifndef MF_GRAYED
#define MF_GRAYED 1
#endif
#ifndef MF_MENUBARBREK
#define MF_MENUBARBREK 0x0020
#endif
#ifndef MF_MENUBREAK
#define MF_MENUBREAK 0x0040
#endif
#ifndef MF_OWNERDRAW
#define MF_OWNERDRAW 0x0100
#endif
#ifndef MF_POPUP
#define MF_POPUP 0x0010
#endif
#ifndef MF_SEPARATOR
#define MF_SEPARATOR 0x0800
#endif
#ifndef MF_STRING
#define MF_STRING 0
#endif
#ifndef MF_UNCHECKED
/* Menu item is not checked. */
#define MF_UNCHECKED 0
#endif
#ifndef MF_DEFAULT
#define MF_DEFAULT 0x1000
#endif
#ifndef MF_SYSMENU
/* Not sure what menu flag SYSMENU is for */
#define MF_SYSMENU 0x2000
#endif

/*
#ifndef MF_
#define MF_ 0
#endif
*/
// $Log: $
