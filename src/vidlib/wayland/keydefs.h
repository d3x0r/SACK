#ifndef __KEYS_DEFINED__
#define __KEYS_DEFINED__

RENDER_NAMESPACE

enum KeyDataEventType {
    KEYDATA
 , KEYDATA_DEFINED // do this stroke?  
 , KEYTRIGGER
};  


// KeyDefineFlag_* are options for flags in the static
// keyboard state... some keys are not redefinable...
#define KDF_NODEFINE 0x01 // set to disallow key redefinition
#define KDF_NOREDEF  0x10
#define KDF_CAPSKEY  0x02 // key is sensitive to capslock state
#define KDF_NUMKEY   0x04 // key is sensitive to numlock state
// the numpad returns num0 - ...
// or numpad returns home/end/pgdn etc...

//#define KDF_NUMKEY   0x04 // Key is sensitive to numlock state
#define KDF_NOKEY    0x08 // no keydef here...

#define KDF_UPACTION 0x20 // action is called on key release


#define KEYMOD_NORMAL 0
#define KEYMOD_SHIFT  KEY_MOD_SHIFT
#define KEYMOD_CTRL   KEY_MOD_CTRL
#define KEYMOD_ALT    KEY_MOD_ALT
#define KEYMOD_ANY    ((KEY_MOD_SHIFT|KEY_MOD_CTRL|KEY_MOD_ALT) + 1)


RENDER_NAMESPACE_END

#endif
//--------------------------------------------------------------------------
//
// $Log: keydefs.h,v $
// Revision 1.1  2004/04/27 09:55:11  d3x0r
// Add keydef to keyhandler path
//
//
