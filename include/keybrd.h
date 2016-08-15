#ifndef KEYBOARD_DEFINITION
#define KEYBOARD_DEFINITION

#include <render.h>

#ifdef __cplusplus
#define _RENDER_KEYBOARD_NAMESPACE namespace keyboard {
#define _RENDER_KEYBOARD_NAMESPACE_END }
#else
#define _RENDER_KEYBOARD_NAMESPACE
#define _RENDER_KEYBOARD_NAMESPACE_END
#endif

RENDER_NAMESPACE
   _RENDER_KEYBOARD_NAMESPACE

			/* Keyboard state tracking structure... not very optimal...
			   \internal usage might be different.                      */
			enum KeyUpDownState {
KEYISUP   =2,
KEYISDOWN =1
			};

/* <combine sack::image::render::keyboard::keyboard_tag>
   
   \ \                                                   */
typedef struct keyboard_tag KEYBOARD;
/* <combine sack::image::render::keyboard::keyboard_tag>
   
   \ \                                                   */
typedef struct keyboard_tag *PKEYBOARD;
struct keyboard_tag
{
#define NUM_KEYS 256  
   /* one byte index... more than sufficient
      
      if character in array is '1' key is down, '2' key is up. */
   char keyupdown[NUM_KEYS];
   /* Indicator that the key is a double-tap, not just a single.
      "!! is different that "!" "!                               */
   char keydouble[NUM_KEYS];
   /* time of the last key event */
   unsigned int  keytime[NUM_KEYS];
   /* I'm not sure, maybe it's the printable key char? */
		unsigned char key[NUM_KEYS];
#if 0
	// void (*Proc)(uintptr_t psv)[NUM_KEYS][8];
#endif
};

_RENDER_KEYBOARD_NAMESPACE_END
RENDER_NAMESPACE_END

#ifdef __cplusplus
#  ifdef _D3D_DRIVER
     using namespace sack::image::render::d3d::keyboard;
#  elif defined( _D3D10_DRIVER )
     using namespace sack::image::render::d3d10::keyboard;
#  elif defined( _D3D11_DRIVER )
     using namespace sack::image::render::d3d11::keyboard;
#  else
     using namespace sack::image::render::keyboard;
#  endif
#endif
//#include "vidlib.h"

	// some common things which are specific to this
   // library, and independant of implementation (so far)
#define KEY_MOD_SHIFT 1
#define KEY_MOD_CTRL  2
#define KEY_MOD_ALT   4
// call trigger on release also...
#define KEY_MOD_RELEASE  8
#define KEY_MOD_ALL_CHANGES  16 // application wants both press and release events.
#define KEY_MOD_EXTENDED 32 // key match must be extended also... (extra arrow keys for instance.. what about SDL)

#define KEY_PRESSED         0x80000000
#define IsKeyPressed( keycode ) ( (keycode) & 0x80000000 )
#define KEY_ALT_DOWN        0x40000000
#define KEY_CONTROL_DOWN    0x20000000
#define KEY_SHIFT_DOWN      0x10000000
#define KEY_MOD_DOWN (KEY_ALT_DOWN|KEY_CONTROL_DOWN)
#define KEY_ALPHA_LOCK_ON   0x08000000
#define KEY_NUM_LOCK_ON     0x04000000

#define KEY_MOD(key)        ( ( (key) & 0x70000000 ) >> 28 )
#define KEY_REAL_CODE(key)  ( ( (key) & 0x00FF0000 ) >> 16 )
#define KEY_CODE(key)       ( (key) & 0xFF )
#define IsKeyExtended(key)  ( ( (key) & 0x00000100 ) >> 8 )

#if defined( _WIN32 ) || defined( WIN32 ) || defined( __CYGWIN__ ) || defined( USE_WIN32_KEY_DEFINES )
// mirrored KEY_ definitions from allegro.H library....
//#include <windows.h>
#include <stdhdrs.h>

#define BIT_7           0x80

#define KEY_TAB          9
#define KEY_CENTER       12
#define KEY_PAD5         12
#define KEY_ENTER        13
#define KEY_LSHIFT       16
#define KEY_SHIFT        16
#define KEY_LEFT_SHIFT   0x10
#define KEY_RIGHT_SHIFT  0x10 // maybe?
#define KEY_SHIFT_LEFT KEY_LEFT_SHIFT
#define KEY_SHIFT_RIGHT KEY_RIGHT_SHIFT
#define KEY_CTRL         17
#define KEY_CONTROL      17
#define KEY_LEFT_CONTROL  17
#define KEY_RIGHT_CONTROL 17
#define KEY_ALT          18 // can't get usually under windows?(keyhook!)
#define KEY_LEFT_ALT      18
#define KEY_RIGHT_ALT     18
#define KEY_CAPS_LOCK    20
#define KEY_ESC          27
#define KEY_ESCAPE       27
#define KEY_PGUP         33
#define KEY_PAGE_UP     KEY_PGUP
#define KEY_PGDN         34
#define KEY_PAGE_DOWN   KEY_PGDN
#define KEY_END          35
#define KEY_HOME         36
#define KEY_LEFT         37
#define KEY_UP           38
#define KEY_RIGHT        39
#define KEY_DOWN         40
#define KEY_GRAY_UP  38
#define KEY_GRAY_LEFT   37
#define KEY_GRAY_RIGHT  39
#define KEY_GRAY_DOWN    40
//#define KEY_GRAY_UP      BIT_7+0x48
#define KEY_GRAY_PGUP   BIT_7+0x49
#define KEY_GRAY_MINUS  BIT_7+0x4A
//#define KEY_GRAY_LEFT BIT_7+0x4B
//#define KEY_GRAY_RIGHT   BIT_7+0x4D
#define KEY_GRAY_PLUS   BIT_7+0x4E
#define KEY_GRAY_END    BIT_7+0x4F
#define KEY_PAD_PLUS   BIT_7+0x4E
//#define KEY_GRAY_DOWN BIT_7+0x50
#define KEY_GRAY_PGDN   BIT_7+0x51
#define KEY_GRAY_INS    BIT_7+0x52
#define KEY_GRAY_DEL    BIT_7+0x53
#define KEY_GRAY_DELETE    BIT_7+0x53
#define KEY_GREY_DELETE    BIT_7+0x53
#define KEY_INSERT       45
#define KEY_DEL          46
#define KEY_DELETE       KEY_DEL

#define KEY_PRINT_SCREEN1  VK_PRINT
#define KEY_PRINT_SCREEN2  VK_SNAPSHOT

#define KEY_WINDOW_2     0x50 // windows keys keys
#define KEY_WINDOW_1     0x5c // windows keys keys

#define KEY_GRAY_STAR     0x6a
#define KEY_PLUS_PAD     0x6b
//#define KEY_GRAY_MINUS    0x6d
#define KEY_GRAY_SLASH    VK_OEM_5

//#define KEY_GRAY_PLUS     107    

#define KEY_NUM_LOCK      VK_NUMLOCK 
#define KEY_SCROLL_LOCK VK_SCROLL 
#define KEY_SLASH        VK_OEM_2
#define KEY_BACKSPACE   '\b'
#define KEY_SPACE        ' '
#define KEY_COMMA      0xBC
#define KEY_STOP       0xBE // should be some sort of VK_ definitions....
#define KEY_PERIOD     KEY_STOP
#define KEY_A         'A'
#define KEY_B         'B'
#define KEY_C         'C'
#define KEY_D         'D'
#define KEY_E         'E'
#define KEY_F         'F'
#define KEY_G         'G'
#define KEY_H         'H'
#define KEY_I         'I'
#define KEY_J         'J'
#define KEY_K         'K'
#define KEY_L         'L'

#define KEY_F12  VK_F12
#define KEY_F11  VK_F11
#define KEY_F10  VK_F10
#define KEY_F9  VK_F9
#define KEY_F8  VK_F8
#define KEY_F7  VK_F7
#define KEY_F6  VK_F6
#define KEY_F5  VK_F5
#define KEY_F4  VK_F4
#define KEY_F3  VK_F3
#define KEY_F2  VK_F2
#define KEY_F1  VK_F1

#define KEY_M        77
#define KEY_N         78
#define KEY_O         79
#define KEY_P        80
#define KEY_Q         'Q'
#define KEY_R         'R'
#define KEY_S         'S'
#define KEY_T         'T'
#define KEY_U         'U'
#define KEY_V         'V'
#define KEY_W         'W'
#define KEY_X         'X'
#define KEY_Y         'Y'
#define KEY_Z         'Z'


#define KEY_1         '1'
#define KEY_2         '2'
#define KEY_3         '3'
#define KEY_4         '4'
#define KEY_5         '5'
#define KEY_6         '6'
#define KEY_7         '7'
#define KEY_8         '8'
#define KEY_9         '9'
#define KEY_0         '0'
#define KEY_MINUS    KEY_DASH


#ifndef VK_OEM_1
// native windows OEM definitions
#define VK_OEM_1   186

#define VK_OEM_2   191
#define VK_OEM_4   219
#define VK_OEM_5   220
#define VK_OEM_6   221
#define VK_OEM_7   222
#define VK_OEM_MINUS  189
#define VK_OEM_PLUS    187
#endif


#define KEY_SEMICOLON     VK_OEM_1
#define KEY_QUOTE         VK_OEM_7
#define KEY_LEFT_BRACKET  VK_OEM_4
#define KEY_RIGHT_BRACKET VK_OEM_6
#define KEY_BACKSLASH     VK_OEM_5
//'-'
#define KEY_DASH     VK_OEM_MINUS 
#define KEY_EQUAL    VK_OEM_PLUS
#define KEY_EQUALS   KEY_EQUAL
#define KEY_ACCENT 192
#define KEY_GRAVE  KEY_ACCENT
#define KEY_APOSTROPHE  KEY_ACCENT

#define KEY_F1  VK_F1
#define KEY_F2  VK_F2
#define KEY_F3  VK_F3
#define KEY_F4  VK_F4
#define KEY_F5  VK_F5
#define KEY_F6  VK_F6
#define KEY_F7  VK_F7
#define KEY_F8  VK_F8
#define KEY_F9  VK_F9
#define KEY_F10  VK_F10
#define KEY_F1  VK_F1


#define VK_NUMPAD0        0x60
#define VK_NUMPAD1        0x61
#define VK_NUMPAD2        0x62
#define VK_NUMPAD3        0x63
#define VK_NUMPAD4        0x64
#define VK_NUMPAD5        0x65
#define VK_NUMPAD6        0x66
#define VK_NUMPAD7        0x67
#define VK_NUMPAD8        0x68
#define VK_NUMPAD9        0x69
#define VK_MULTIPLY       0x6A
#define VK_ADD            0x6B
#define VK_SEPARATOR      0x6C
#define VK_SUBTRACT       0x6D
#define VK_DECIMAL        0x6E
#define VK_DIVIDE         0x6F

#define KEY_PAD_MULT VK_MULTIPLY
#define KEY_PAD_DOT VK_DECIMAL
#define KEY_PAD_DIV VK_DIVIDE
#define KEY_PAD_0 VK_NUMPAD0
#define KEY_GREY_INSERT VK_NUMPAD0
#define KEY_PAD_1 VK_NUMPAD1
#define KEY_PAD_2 VK_NUMPAD2
#define KEY_PAD_3 VK_NUMPAD3
#define KEY_PAD_4 VK_NUMPAD4
#define KEY_PAD_5 VK_NUMPAD5
#define KEY_PAD_6 VK_NUMPAD6
#define KEY_PAD_7 VK_NUMPAD7
#define KEY_PAD_8 VK_NUMPAD8
#define KEY_PAD_9 VK_NUMPAD9
#define KEY_PAD_ENTER VK_RETURN
#define KEY_PAD_DELETE VK_SEPARATOR
#define KEY_PAD_MINUS VK_SUBTRACT

#endif


// if any key...
#if !defined( KEY_1 )

#if defined( __ANDROID__ )

#include <android/keycodes.h>

#define KEY_SHIFT        AKEYCODE_SHIFT_LEFT
#define KEY_LEFT_SHIFT   AKEYCODE_SHIFT_LEFT
#define KEY_RIGHT_SHIFT  AKEYCODE_SHIFT_RIGHT // maybe?

#ifndef AKEYCODE_CTRL_LEFT
#define AKEYCODE_CTRL_LEFT 113
#endif
#ifndef AKEYCODE_CTRL_RIGHT
#define AKEYCODE_CTRL_RIGHT 114
#endif

#define KEY_CTRL          AKEYCODE_CTRL_LEFT
#define KEY_CONTROL       AKEYCODE_CTRL_LEFT
#define KEY_LEFT_CONTROL  AKEYCODE_CTRL_LEFT
#define KEY_RIGHT_CONTROL AKEYCODE_CTRL_RIGHT

#define KEY_ALT           AKEYCODE_ALT_LEFT // can't get usually under windows?(keyhook!)
#define KEY_LEFT_ALT      AKEYCODE_ALT_LEFT
#define KEY_RIGHT_ALT     AKEYCODE_ALT_RIGHT

#ifndef AKEYCODE_CAPS_LOCK
#define AKEYCODE_CAPS_LOCK 115
#endif
#define KEY_CAPS_LOCK     AKEYCODE_CAPS_LOCK
#define KEY_NUM_LOCK      0
#ifndef AKEYCODE_SCROLL_LOCK
#define AKEYCODE_SCROLL_LOCK 116
#endif
#define KEY_SCROLL_LOCK   AKEYCODE_SCROLL_LOCK // unsure about this
#ifndef AKEYCODE_ESCAPE
#define AKEYCODE_ESCAPE 111
#endif
#define KEY_ESC           AKEYCODE_ESCAPE
#define KEY_ESCAPE        AKEYCODE_ESCAPE

#ifndef AKEYCODE_MOVE_HOME
#define AKEYCODE_MOVE_HOME 122
#endif
#ifndef AKEYCODE_MOVE_END
#define AKEYCODE_MOVE_END 123
#endif
#define KEY_HOME          AKEYCODE_MOVE_HOME
#define KEY_PAD_HOME      AKEYCODE_MOVE_HOME
#define KEY_PAD_7         0
#define KEY_GREY_HOME     0

#define KEY_UP            AKEYCODE_DPAD_UP
#define KEY_PAD_8         0
#define KEY_PAD_UP        0
#define KEY_GREY_UP       0

#define KEY_PGUP          0
#define KEY_PAD_9         0
#define KEY_PAD_PGUP      0
#define KEY_GREY_PGUP     0

#define KEY_LEFT          AKEYCODE_DPAD_LEFT
#define KEY_PAD_4         0
#define KEY_PAD_LEFT      0
#define KEY_GREY_LEFT     0

#define KEY_CENTER        AKEYCODE_DPAD_CENTER
#define KEY_PAD_5         0
#define KEY_PAD_CENTER    0
#define KEY_GREY_CENTER   0

#define KEY_RIGHT         AKEYCODE_DPAD_RIGHT
#define KEY_PAD_6         0
#define KEY_PAD_RIGHT     0
#define KEY_GREY_RIGHT    0

#define KEY_END           AKEYCODE_MOVE_END
#define KEY_PAD_1         0
#define KEY_PAD_END       0
#define KEY_GREY_END      0

#define KEY_DOWN          AKEYCODE_DPAD_DOWN
#define KEY_PAD_2         0
#define KEY_PAD_DOWN      0
#define KEY_GREY_DOWN     0

#define KEY_PGDN          0
#define KEY_PAD_3         0
#define KEY_PAD_PGDN      0
#define KEY_GREY_PGDN     0

#define KEY_INSERT        0
#define KEY_PAD_0         0
#define KEY_PAD_INSERT    0
#define KEY_GREY_INSERT   0

#define KEY_DELETE        0
#define KEY_PAD_DOT       0
#define KEY_PAD_DELETE    0
#define KEY_GREY_DELETE   0

#define KEY_PLUS          0
#define KEY_PAD_PLUS      0
#define KEY_GREY_PLUS     0

#define KEY_MINUS         0
#define KEY_PAD_MINUS     0
#define KEY_GREY_MINUS    0

#define KEY_MULT          0
#define KEY_PAD_MULT      0
#define KEY_GREY_MULT     0

#define KEY_DIV           0
#define KEY_PAD_DIV       0
#define KEY_GREY_DIV      0

#define KEY_ENTER         AKEYCODE_ENTER
#define KEY_PAD_ENTER     AKEYCODE_ENTER
#define KEY_NORMAL_ENTER  AKEYCODE_ENTER

#define KEY_WINDOW_1      0 // windows keys keys
#define KEY_WINDOW_2      0 // windows keys keys


#define KEY_TAB           AKEYCODE_TAB

#define KEY_SLASH         AKEYCODE_SLASH
#define KEY_BACKSPACE     AKEYCODE_DEL
#define KEY_SPACE         AKEYCODE_SPACE
#define KEY_COMMA         AKEYCODE_COMMA
#define KEY_STOP          AKEYCODE_PERIOD // should be some sort of VK_ definitions....
#define KEY_PERIOD        AKEYCODE_PERIOD
#define KEY_SEMICOLON     AKEYCODE_SEMICOLON
#define KEY_QUOTE         AKEYCODE_APOSTROPHE
#define KEY_LEFT_BRACKET  AKEYCODE_LEFT_BRACKET
#define KEY_RIGHT_BRACKET AKEYCODE_RIGHT_BRACKET
#define KEY_BACKSLASH     AKEYCODE_BACKSLASH
#define KEY_DASH          AKEYCODE_MINUS
#define KEY_EQUAL         AKEYCODE_EQUALS
#define KEY_ACCENT        AKEYCODE_GRAVE

#define KEY_1         AKEYCODE_1
#define KEY_2         AKEYCODE_2
#define KEY_3         AKEYCODE_3
#define KEY_4         AKEYCODE_4
#define KEY_5         AKEYCODE_5
#define KEY_6         AKEYCODE_6
#define KEY_7         AKEYCODE_7
#define KEY_8         AKEYCODE_8
#define KEY_9         AKEYCODE_9
#define KEY_0         AKEYCODE_0

#define KEY_F1        0
#define KEY_F2        0
#define KEY_F3        0
#define KEY_F4        0
#define KEY_F5        0
#define KEY_F6        0
#define KEY_F7        0
#define KEY_F8        0
#define KEY_F9        0
#define KEY_F10       0
#define KEY_F11       0
#define KEY_F12       0


#define KEY_A   AKEYCODE_A
#define KEY_B   AKEYCODE_B
#define KEY_C   AKEYCODE_C
#define KEY_D   AKEYCODE_D
#define KEY_E   AKEYCODE_E
#define KEY_F   AKEYCODE_F
#define KEY_G   AKEYCODE_G
#define KEY_H   AKEYCODE_H
#define KEY_I   AKEYCODE_I
#define KEY_J   AKEYCODE_J
#define KEY_K   AKEYCODE_K
#define KEY_L   AKEYCODE_L
#define KEY_M   AKEYCODE_M
#define KEY_N   AKEYCODE_N
#define KEY_O   AKEYCODE_O
#define KEY_P   AKEYCODE_P
#define KEY_Q   AKEYCODE_Q
#define KEY_R   AKEYCODE_R
#define KEY_S   AKEYCODE_S
#define KEY_T   AKEYCODE_T
#define KEY_U   AKEYCODE_U
#define KEY_V   AKEYCODE_V
#define KEY_W   AKEYCODE_W
#define KEY_X   AKEYCODE_X
#define KEY_Y   AKEYCODE_Y
#define KEY_Z   AKEYCODE_Z

#elif defined( __LINUX__ )

	  //#define USE_SDL_KEYSYM
// ug - KEYSYMS are too wide...
// so - we fall back to x scancode tables - and translate sym to these
// since the scancodes which come from X are not the same as from console Raw
// but - perhaps we should re-translate these to REAL scancodes... but in either
// case - these fall to under 256 characters, and can therefore be used...

#define USE_X_RAW_SCANCODES


#ifdef USE_X_RAW_SCANCODES

#define KEY_SHIFT        0xFF
#define KEY_LEFT_SHIFT   50
#define KEY_RIGHT_SHIFT  62 // maybe?

#define KEY_CTRL          0xFE
#define KEY_CONTROL       0xFE
#define KEY_LEFT_CONTROL  37
#define KEY_RIGHT_CONTROL 109

#define KEY_ALT           0xFD // can't get usually under windows?(keyhook!)
#define KEY_LEFT_ALT      64
#define KEY_RIGHT_ALT     113

#define KEY_CAPS_LOCK     66
#define KEY_NUM_LOCK      77
#define KEY_SCROLL_LOCK   78 // unsure about this
#define KEY_ESC           9
#define KEY_ESCAPE        9

#define KEY_HOME          0xFC
#define KEY_PAD_HOME      79
#define KEY_PAD_7         79
#define KEY_GREY_HOME     97

#define KEY_UP            0xFB
#define KEY_PAD_8         80
#define KEY_PAD_UP        80
#define KEY_GREY_UP       98

#define KEY_PGUP          0xFA
#define KEY_PAGE_UP       KEY_PGUP
#define KEY_PAD_9         81
#define KEY_PAD_PGUP      81
#define KEY_GREY_PGUP     99

#define KEY_LEFT          0xF9
#define KEY_PAD_4         83
#define KEY_PAD_LEFT      83
#define KEY_GREY_LEFT     100

#define KEY_CENTER        0xF8
#define KEY_PAD_5         84
#define KEY_PAD_CENTER    84
#define KEY_GREY_CENTER   0

#define KEY_RIGHT         0xF7
#define KEY_PAD_6         85
#define KEY_PAD_RIGHT     85
#define KEY_GREY_RIGHT    102

#define KEY_END           0xF6
#define KEY_PAD_1         87
#define KEY_PAD_END       87
#define KEY_GREY_END      103

#define KEY_DOWN          0xF5
#define KEY_PAD_2         88
#define KEY_PAD_DOWN      88
#define KEY_GREY_DOWN     104

#define KEY_PGDN          0xF4
#define KEY_PAGE_DOWN     KEY_PGDN
#define KEY_PAD_3         89
#define KEY_PAD_PGDN      89
#define KEY_GREY_PGDN     105


#define KEY_INSERT        0xF3
#define KEY_PAD_0         90
#define KEY_PAD_INSERT    90
#define KEY_GREY_INSERT   106

#define KEY_DELETE        0xF2
#define KEY_DEL           KEY_DELETE
#define KEY_PAD_DOT       91
#define KEY_PAD_DELETE    91
#define KEY_GREY_DELETE   107

#define KEY_PLUS          0xF1
#define KEY_PAD_PLUS      86
#define KEY_GREY_PLUS     0

#define KEY_MINUS         0xF0
#define KEY_PAD_MINUS     82
#define KEY_GREY_MINUS    0

#define KEY_MULT          0xEF
#define KEY_PAD_MULT      63
#define KEY_GREY_MULT     0

#define KEY_DIV           0xEE
#define KEY_PAD_DIV       112
#define KEY_GREY_DIV      0

#define KEY_ENTER         0xED
#define KEY_PAD_ENTER     108
#define KEY_NORMAL_ENTER  36

#define KEY_WINDOW_1      115 // windows keys keys
#define KEY_WINDOW_2      117 // windows keys keys


#define KEY_TAB           23

#define KEY_SLASH         61
#define KEY_BACKSPACE     22
#define KEY_SPACE         65
#define KEY_COMMA         59
#define KEY_STOP          60 // should be some sort of VK_ definitions....
#define KEY_PERIOD        KEY_STOP
#define KEY_SEMICOLON     47
#define KEY_QUOTE         48
#define KEY_LEFT_BRACKET  34
#define KEY_RIGHT_BRACKET 35
#define KEY_BACKSLASH     51
#define KEY_DASH          20
#define KEY_EQUAL         21
#define KEY_EQUALS       KEY_EQUAL
#define KEY_ACCENT        49
#define KEY_APOSTROPHE    KEY_QUOTE
#define KEY_GRAVE        KEY_ACCENT
#define KEY_SHIFT_LEFT   KEY_LEFT_SHIFT
#define KEY_SHIFT_RIGHT  KEY_RIGHT_SHIFT

#define KEY_1         10
#define KEY_2         11
#define KEY_3         12
#define KEY_4         13
#define KEY_5         14
#define KEY_6         15
#define KEY_7         16
#define KEY_8         17
#define KEY_9         18
#define KEY_0         19

#define KEY_F1        67
#define KEY_F2        68
#define KEY_F3        69
#define KEY_F4        70
#define KEY_F5        71
#define KEY_F6        72
#define KEY_F7        73
#define KEY_F8        74
#define KEY_F9        75
#define KEY_F10       76
#define KEY_F11       95
#define KEY_F12       96


#define KEY_A         38
#define KEY_B         56
#define KEY_C         54
#define KEY_D         40
#define KEY_E         26
#define KEY_F         41
#define KEY_G         42
#define KEY_H         43
#define KEY_I         31
#define KEY_J         44
#define KEY_K         45
#define KEY_L         46
#define KEY_M         58
#define KEY_N         57
#define KEY_O         32
#define KEY_P         33
#define KEY_Q         24
#define KEY_R         27
#define KEY_S         39
#define KEY_T         28
#define KEY_U         30
#define KEY_V         55
#define KEY_W         25
#define KEY_X         53
#define KEY_Y         29
#define KEY_Z         52

#elif defined( USE_SDL_KEYSYM )
#include <SDL.h>
#define KEY_SHIFT        0xFF
#define KEY_LEFT_SHIFT   SDLK_LSHIFT
#define KEY_RIGHT_SHIFT  SDLK_RSHIFT

#define KEY_CTRL          0xFE
#define KEY_CONTROL       0xFE
#define KEY_LEFT_CONTROL  SDLK_LCTRL
#define KEY_RIGHT_CONTROL SDLK_RCTRL

#define KEY_ALT           0xFD // can't get usually under windows?(keyhook!)
#define KEY_LEFT_ALT      SDLK_LALT
#define KEY_RIGHT_ALT     SDLK_RALT

#define KEY_CAPS_LOCK     SDLK_CAPSLOCK
#define KEY_NUM_LOCK      SDLK_NUMLOCK
#define KEY_SCROLL_LOCK   SDLK_SCROLLOCK
#define KEY_ESC           SDLK_ESCAPE
#define KEY_ESCAPE        SDLK_ESCAPE

#define KEY_HOME          0xFC
#define KEY_PAD_HOME      SDLK_KP7
#define KEY_PAD_7         SDLK_KP7
#define KEY_GREY_HOME     SDLK_HOME

#define KEY_UP            0xFB
#define KEY_PAD_8         SDLK_KP8
#define KEY_PAD_UP        SDLK_KP8
#define KEY_GREY_UP       SDLK_UP

#define KEY_PGUP          0xFA
#define KEY_PAD_9         SDLK_KP9
#define KEY_PAD_PGUP      SDLK_KP9
#define KEY_GREY_PGUP     SDLK_PAGEUP

#define KEY_LEFT          0xF9
#define KEY_PAD_4         SDLK_KP4
#define KEY_PAD_LEFT      SDLK_KP4
#define KEY_GREY_LEFT     SDLK_LEFT

#define KEY_CENTER        0xF8
#define KEY_PAD_5         SDLK_KP5
#define KEY_PAD_CENTER    SDLK_KP5
#define KEY_GREY_CENTER   0

#define KEY_RIGHT         0xF7
#define KEY_PAD_6         SDLK_KP6
#define KEY_PAD_RIGHT     SDLK_KP6
#define KEY_GREY_RIGHT    SDLK_RIGHT

#define KEY_END           0xF6
#define KEY_PAD_1         SDLK_KP1
#define KEY_PAD_END       SDLK_KP1
#define KEY_GREY_END      SDLK_END

#define KEY_DOWN          0xF5
#define KEY_PAD_2         SDLK_KP2
#define KEY_PAD_DOWN      SDLK_KP2
#define KEY_GREY_DOWN     SDLK_DOWN

#define KEY_PGDN          0xF4
#define KEY_PAD_3         SDLK_KP3
#define KEY_PAD_PGDN      SDLK_KP3
#define KEY_GREY_PGDN     SDLK_PAGEDN

#define KEY_INSERT        0xF3
#define KEY_PAD_0         SDLK_KP0
#define KEY_PAD_INSERT    SDLK_KP0
#define KEY_GREY_INSERT   SDLK_INSERT

#define KEY_DELETE        0xF2
#define KEY_PAD_DOT       SDLK_KP_PERIOD
#define KEY_PAD_DELETE    SDLK_KP_PERIOD
#define KEY_GREY_DELETE   SDLK_DELETE

#define KEY_PLUS          0xF1
#define KEY_PAD_PLUS      SDLK_KP_PLUS
#define KEY_GREY_PLUS     0

#define KEY_MINUS         0xF0
#define KEY_PAD_MINUS     SDLK_KP_MINUS
#define KEY_GREY_MINUS    0

#define KEY_MULT          0xEF
#define KEY_PAD_MULT      SDLK_KP_MULTIPLY
#define KEY_GREY_MULT     0

#define KEY_DIV           0xEE
#define KEY_PAD_DIV       SDLK_KP_DIVIDE
#define KEY_GREY_DIV      0

#define KEY_ENTER         0xED
#define KEY_PAD_ENTER     SDLK_KP_ENTER
#define KEY_NORMAL_ENTER  SDLK_RETURN

#define KEY_WINDOW_1      115 // windows keys keys
#define KEY_WINDOW_2      117 // windows keys keys


#define KEY_TAB           SDLK_TAB

#define KEY_SLASH         SDLK_SLASH
#define KEY_BACKSPACE     SDLK_BACKSPACE
#define KEY_SPACE         SDLK_SPACE
#define KEY_COMMA         SDLK_COMMA
#define KEY_STOP          SDLK_PERIOD // should be some sort of VK_ definitions....
#define KEY_PERIOD        KEY_STOP
#define KEY_SEMICOLON     SDLK_SEMICOLON
#define KEY_QUOTE         SDLK_QUOTE
#define KEY_LEFT_BRACKET  SDLK_LEFTBRACKET
#define KEY_RIGHT_BRACKET SDLK_RIGHTBRACKET
#define KEY_BACKSLASH     SDLK_BACKSLASH
#define KEY_DASH          SDLK_MINUS
#define KEY_EQUAL         SDLK_EQUALS
#define KEY_ACCENT        SDLK_BACKQUOTE // grave

#define KEY_1         SDLK_1
#define KEY_2         SDLK_2
#define KEY_3         SDLK_3
#define KEY_4         SDLK_4
#define KEY_5         SDLK_5
#define KEY_6         SDLK_6
#define KEY_7         SDLK_7
#define KEY_8         SDLK_8
#define KEY_9         SDLK_9
#define KEY_0         SDLK_0

#define KEY_F1        SDLK_F1
#define KEY_F2        SDLK_F2
#define KEY_F3        SDLK_F3
#define KEY_F4        SDLK_F4
#define KEY_F5        SDLK_F5
#define KEY_F6        SDLK_F6
#define KEY_F7        SDLK_F7
#define KEY_F8        SDLK_F8
#define KEY_F9        SDLK_F9
#define KEY_F10       SDLK_F10
#define KEY_F11       SDLK_F11
#define KEY_F12       SDLK_F12

#define KEY_A         SDLK_A
#define KEY_B         SDLK_B
#define KEY_C         SDLK_C
#define KEY_D         SDLK_D
#define KEY_E         SDLK_E
#define KEY_F         SDLK_F
#define KEY_G         SDLK_G
#define KEY_H         SDLK_H
#define KEY_I         SDLK_I
#define KEY_J         SDLK_J
#define KEY_K         SDLK_K
#define KEY_L         SDLK_L
#define KEY_M         SDLK_M
#define KEY_N         SDLK_N
#define KEY_O         SDLK_O
#define KEY_P         SDLK_P
#define KEY_Q         SDLK_Q
#define KEY_R         SDLK_R
#define KEY_S         SDLK_S
#define KEY_T         SDLK_T
#define KEY_U         SDLK_U
#define KEY_V         SDLK_V
#define KEY_W         SDLK_W
#define KEY_X         SDLK_X
#define KEY_Y         SDLK_Y
#define KEY_Z         SDLK_Z

#elif defined( USE_RAW_SCANCODE )
#error RAW_SCANCODES have not been defined yet.
#define KEY_SHIFT        0xFF
#define KEY_LEFT_SHIFT   50
#define KEY_RIGHT_SHIFT  62 // maybe?

#define KEY_CTRL          0xFE
#define KEY_CONTROL       0xFE
#define KEY_LEFT_CONTROL  37
#define KEY_RIGHT_CONTROL 109

#define KEY_ALT           0xFD // can't get usually under windows?(keyhook!)
#define KEY_LEFT_ALT      64
#define KEY_RIGHT_ALT     113

#define KEY_CAPS_LOCK     66
#define KEY_NUM_LOCK      77
#define KEY_SCROLL_LOCK   78 // unsure about this
#define KEY_ESC           9
#define KEY_ESCAPE        9

#define KEY_HOME          0xFC
#define KEY_PAD_HOME      79
#define KEY_PAD_7         79
#define KEY_GREY_HOME     97

#define KEY_UP            0xFB
#define KEY_PAD_8         80
#define KEY_PAD_UP        80
#define KEY_GREY_UP       98

#define KEY_PGUP          0xFA
#define KEY_PAD_9         81
#define KEY_PAD_PGUP      81
#define KEY_GREY_PGUP     99

#define KEY_LEFT          0xF9
#define KEY_PAD_4         83
#define KEY_PAD_LEFT      83
#define KEY_GREY_LEFT     100

#define KEY_CENTER        0xF8
#define KEY_PAD_5         84
#define KEY_PAD_CENTER    84
#define KEY_GREY_CENTER   0

#define KEY_RIGHT         0xF7
#define KEY_PAD_6         85
#define KEY_PAD_RIGHT     85
#define KEY_GREY_RIGHT    102

#define KEY_END           0xF6
#define KEY_PAD_1         87
#define KEY_PAD_END       87
#define KEY_GREY_END      103

#define KEY_DOWN          0xF5
#define KEY_PAD_2         88
#define KEY_PAD_DOWN      88
#define KEY_GREY_DOWN     104

#define KEY_PGDN          0xF4
#define KEY_PAD_3         89
#define KEY_PAD_PGDN      89
#define KEY_GREY_PGDN     105

#define KEY_INSERT        0xF3
#define KEY_PAD_0         90
#define KEY_PAD_INSERT    90
#define KEY_GREY_INSERT   106

#define KEY_DELETE        0xF2
#define KEY_PAD_DOT       91
#define KEY_PAD_DELETE    91
#define KEY_GREY_DELETE   107

#define KEY_PLUS          0xF1
#define KEY_PAD_PLUS      86
#define KEY_GREY_PLUS     0

#define KEY_MINUS         0xF0
#define KEY_PAD_MINUS     82
#define KEY_GREY_MINUS    0

#define KEY_MULT          0xEF
#define KEY_PAD_MULT      63
#define KEY_GREY_MULT     0

#define KEY_DIV           0xEE
#define KEY_PAD_DIV       112
#define KEY_GREY_DIV      0

#define KEY_ENTER         0xED
#define KEY_PAD_ENTER     108
#define KEY_NORMAL_ENTER  36

#define KEY_WINDOW_1      115 // windows keys keys
#define KEY_WINDOW_2      117 // windows keys keys


#define KEY_TAB           23

#define KEY_SLASH         61
#define KEY_BACKSPACE     22
#define KEY_SPACE         65
#define KEY_COMMA         59
#define KEY_STOP          60 // should be some sort of VK_ definitions....
#define KEY_PERIOD        KEY_STOP
#define KEY_SEMICOLON     47
#define KEY_QUOTE         48
#define KEY_LEFT_BRACKET  34
#define KEY_RIGHT_BRACKET 35
#define KEY_BACKSLASH     51
#define KEY_DASH          20
#define KEY_EQUAL         21
#define KEY_ACCENT        49

#define KEY_1         10
#define KEY_2         11
#define KEY_3         12
#define KEY_4         13
#define KEY_5         14
#define KEY_6         15
#define KEY_7         16
#define KEY_8         17
#define KEY_9         18
#define KEY_0         19

#define KEY_F1        67
#define KEY_F2        68
#define KEY_F3        69
#define KEY_F4        70
#define KEY_F5        71
#define KEY_F6        72
#define KEY_F7        73
#define KEY_F8        74
#define KEY_F9        75
#define KEY_F10       76
#define KEY_F11       95
#define KEY_F12       96


#define KEY_A         38
#define KEY_B         56
#define KEY_C         54
#define KEY_D         40
#define KEY_E         26
#define KEY_F         41
#define KEY_G         42
#define KEY_H         43
#define KEY_I         31
#define KEY_J         44
#define KEY_K         45
#define KEY_L         46
#define KEY_M         58
#define KEY_N         57
#define KEY_O         32
#define KEY_P         33
#define KEY_Q         24
#define KEY_R         27
#define KEY_S         39
#define KEY_T         28
#define KEY_U         30
#define KEY_V         55
#define KEY_W         25
#define KEY_X         53
#define KEY_Y         29
#define KEY_Z         52

#endif

#endif
#endif

#elif defined( DEFINE_HARDWARE_SCANCODES )
#ifndef KBD_HPP
#define KBD_HPP

#define KBD_INT            9
#define KBD_EXTENDED_CODE     0xE0
#define LOW_ASCII(asc)     (asc&0x7F)
#define NUM_KEYS        256

#ifdef WIN32
//#define KEY_ESC       27
//#define KEY_LEFT      37
//#define KEY_CENTER    KB_CENTER
//#define KEY_RIGHT     39
//#define KEY_DOWN      40
//#define KEY_GRAY_UP   38
//#define KEY_GRAY_LEFT 37
//#define KEY_GRAY_RIGHT   39
//#define KEY_GRAY_DOWN    40
//#define KEY_LEFT_SHIFT   16
//#define KEY_RIGHT_SHIFT  16
//#define KEY_GRAY_PGUP 33
//#define KEY_GRAY_PGDN 34
//#define KEY_GRAY_INS  45
//#define KEY_GRAY_DEL  46
//#define KEY_P         80
//#define KEY_M         77
#else
#define KEY_ESC       0x01
#define KEY_1         0x02
#define KEY_2         0x03
#define KEY_3         0x04
#define KEY_4         0x05
#define KEY_5         0x06
#define KEY_6         0x07
#define KEY_7         0x08
#define KEY_8         0x09
#define KEY_9         0x0A
#define KEY_0         0x0B
#define KEY_MINUS     0x0C
#define KEY_PLUS         0x0D
#define  KEY_BKSP        0x0E
#define KEY_TAB       0x0F
#define KEY_Q         0x10
#define KEY_W         0x11  
#define KEY_E         0x12
#define KEY_R         0x13
#define KEY_T         0x14
#define KEY_Y         0x15
#define KEY_U         0x16
#define KEY_I         0x17
#define  KEY_O        0x18
#define KEY_P         0x19
#define KEY_BRACK_OPEN   0x1A
#define KEY_BRACK_CLOSE  0x1B
#define KEY_ENTER     0x1C
#define KEY_LEFT_CTRL 0x1D
#define KEY_A         0x1E
#define KEY_S         0x1F
#define KEY_D         0x20
#define KEY_F         0x21
#define KEY_X         0x2D
#define KEY_C         0x2E
#define KEY_V         0x2F
#define KEY_B         0x30
#define KEY_N         0x31
#define KEY_M         0x32
#define KEY_GRAY_SLASH   0x35
#define KEY_RIGHT_SHIFT  0x36
#define KEY_GRAY_STAR 0x37
#define KEY_LEFT_ALT     0x38
#define KEY_SPACE     0x39
#define KEY_CAPS         0x3A
#define KEY_F1        0x3B
#define KEY_F2        0x3C
#define KEY_F3        0x3D
#define KEY_F4        0x3E
#define KEY_F5        0x3F
#define KEY_F6        0x40
#define KEY_F7        0x41
#define KEY_F8        0x42
#define KEY_F9        0x43
#define KEY_F10       0x44
#define KEY_UP        0x48
#define KEY_LEFT      0x4B
#define KEY_CENTER    0x4C
#define KEY_RIGHT     0x4D
#define KEY_DOWN      0x50
#define KEY_DEL       0x53
#define KEY_F11       0x57
#define KEY_F12       0x58
#define KEY_RIGHT_CTRL   BIT_7+0x1D
#define KEY_RIGHT_ALT BIT_7+0x38
#define KEY_GRAY_UP      BIT_7+0x48
#define KEY_GRAY_PGUP BIT_7+0x49
#define KEY_GRAY_MINUS   BIT_7+0x4A
#define KEY_GRAY_LEFT BIT_7+0x4B
#define KEY_GRAY_RIGHT   BIT_7+0x4D
#define KEY_GRAY_PLUS BIT_7+0x4E
#define KEY_GRAY_END     BIT_7+0x4F
#define KEY_GRAY_DOWN BIT_7+0x50
#define KEY_GRAY_PGDN BIT_7+0x51
#define KEY_GRAY_INS     BIT_7+0x52
#define KEY_GRAY_DEL     BIT_7+0x53
#endif

#endif

#endif
// $Log: keybrd.h,v $
// Revision 1.16  2004/08/11 11:41:06  d3x0r
// Begin seperation of key and render
//
// Revision 1.15  2004/06/01 21:53:43  d3x0r
// Fix PUBLIC dfeinitions from Windoze-centric to system nonspecified
//
// Revision 1.14  2004/04/27 04:58:16  d3x0r
// Forgot to macro a function..
//
// Revision 1.13  2004/04/27 03:06:16  d3x0r
// Define F1-F10
//
// Revision 1.12  2004/03/05 23:33:21  d3x0r
// Missing keydefs - may be wrong.
//
// Revision 1.11  2003/03/25 08:38:11  panther
// Add logging
//
