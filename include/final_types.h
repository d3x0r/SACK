//  these are rude defines overloading otherwise very practical types
// but - they have to be dispatched after all standard headers.

#ifndef FINAL_TYPES
#define FINAL_TYPES

#  ifdef __WATCOMC__
#    include <stdio.h>
#  endif //__WATCOMC__

#  ifdef _WIN32

#    include <stdio.h>
#    include <basetsd.h>
#    include <windef.h>
#    include <winbase.h>  // this redefines lprintf sprintf etc... and strsafe is preferred
#    include <winuser.h> // more things that need override by strsafe.h
#    include <tchar.h>
#    ifdef __GNUC__ // added for mingw64 actually
#      undef __CRT__NO_INLINE
#    endif
#    ifndef MINGW_SUX
#      include <strsafe.h>
#    else
#      define STRSAFE_E_INSUFFICIENT_BUFFER  0x8007007AL
#    endif
#  else
#    include <wchar.h>
#  endif

// may consider changing this to uint16_t* for unicode...
#ifdef UNICODE
#  ifndef NO_UNICODE_C
#    define strrchr          wcsrchr
#    define strchr           wcschr
#    define strncpy          wcsncpy
#    ifdef strcpy
#      undef strcpy
#    endif
#    define strcpy           wcscpy
#    define strcmp           wcscmp

#    ifndef __LINUX__
// linux also translates 'i' to 'case' in sack_typelib.h
#      define stricmp          wcsicmp
#      define strnicmp         wcsnicmp
//#  define strlen           mbrlen
#    endif
#    define strlen           wcslen

#    ifdef WIN32
#      define stat(a,b)        _wstat(a,b)
#    else
#    endif
#    define printf           wprintf
#    define fprintf          fwprintf
#    define fputs            fputws
#    define fgets            fgetws
#    define atoi             _wtoi
#    ifdef __WATCOMC__
#      undef atof
#    endif
//#    define atof             _wtof

#    ifdef _MSC_VER
#      ifndef __cplusplus_cli
#        define fprintf   fwprintf
#        define atoi      _wtoi
// define sprintf here.
#      endif
#    endif
#    if defined( _ARM_ ) && defined( WIN32 )
// len should be passed as character count. this was the wrongw ay to default this.
#      define snprintf StringCbPrintf
//#define snprintf StringCbPrintf
#    endif
#  else
//#    define atoi             wtoi

#  endif
#else // not unicode...
#endif

#  ifdef _MSC_VER
#    define snprintf _snprintf
#    define vsnprintf _vsnprintf
#    if defined( _UNICODE )
#      define tnprintf _snwprintf
#      define vtnprintf _vsnwprintf
#    else
#      define tnprintf _snprintf
#      define vtnprintf _vsnprintf
#    endif
#    define snwprintf _snwprintf

#    if defined( _UNICODE ) && !defined( NO_UNICODE_C )
#    define tscanf swscanf_s
#    else
#    define tscanf sscanf_s
#    endif
#    define scanf sscanf_s
#    define swcanf swscanf_s

#  endif // _MSC_VER

#  ifdef  __GNUC__
#      if defined( _UNICODE )
#        define VSNPRINTF_FAILS_RETURN_SIZE
#        define tnprintf  swprintf
#        define vtnprintf vswprintf
#        if !defined( NO_UNICODE_C )
#           define snprintf   swprintf
#           define vsnprintf  vswprintf
//#           define sscanf     swscanf
#        else
#        endif
#      else
#        define tnprintf snprintf
#        define vtnprintf vsnprintf
//#        define snprintf snprintf
//#        define vsnprintf vsnprintf
#    if defined( _UNICODE ) && !defined( NO_UNICODE_C )
#    define tscanf swscanf
#    else
#    define tscanf sscanf
#    endif
#      endif

#  endif // __GNUC__


#  ifdef __WATCOMC__
#      if defined( _UNICODE )
#        define tnprintf  _snwprintf
#        define vtnprintf _vsnwprintf
#        if !defined( NO_UNICODE_C )
#           define snprintf  _snwprintf
#           define vsnprintf _vsnwprintf
#           define sscanf     swscanf
#        else
#        endif
#      else
#         define tnprintf  snprintf
#         define vtnprintf vsnprintf
//#        define snprintf snprintf
//#        define vsnprintf vsnprintf
#      endif
#        define snwprintf  _snwprintf

#  endif // __WATCOMC__

#endif
