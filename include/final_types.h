//  these are rude defines overloading otherwise very practical types
// but - they have to be dispatched after all standard headers.

#ifndef FINAL_TYPES
#define FINAL_TYPES

# ifdef __WATCOMC__
#    include <stdio.h>
#  endif //__WATCOMC__

# ifdef _MSC_VER

#include <stdio.h>
#include <baseTsd.h>
#include <windef.h>
#include <winbase.h>  // this redefines lprintf sprintf etc... and strsafe is preferred
#include <winuser.h> // more things that need override by strsafe.h
#include <tchar.h>
#include <strsafe.h>

#  endif

// may consider changing this to P_16 for unicode...
#ifdef UNICODE
# ifndef NO_UNICODE_C
#  define strrchr          wcsrchr
#  define strchr           wcschr
#  define strncpy          wcsncpy
# ifdef strcpy
#  undef strcpy
# endif
#  define strcpy           wcscpy
#  define strcmp           wcscmp

#  define strlen           wcslen
#  define stricmp          wcsicmp
#  define strnicmp         wcsnicmp
#  define stat(a,b)        _wstat(a,b)
#  define printf           wprintf
#  define fprintf          fwprintf
#  define fputs            fputws
#  define fgets            fgetws
#  define atoi             _wtoi
#  ifdef __WATCOMC__
#    undef atof
#  endif
#  define atof             _wtof

#  ifdef _MSC_VER
#    ifndef __cplusplus_cli
#   define fprintf   fwprintf
#   define atoi      _wtoi
// define sprintf here.
#       endif
#    endif
#    ifdef _ARM_
// len should be passed as character count. this was the wrongw ay to default this.
#      define snprintf StringCbPrintf
//#define snprintf StringCbPrintf
#    endif
#  else
#  endif
#else // not unicode...
#endif

#  ifdef _MSC_VER
#define SUFFER_WITH_NO_SNPRINTF
#    ifndef SUFFER_WITH_NO_SNPRINTF
#      define vnsprintf protable_vsnprintf
//   this one gives deprication warnings
//   #    define vsnprintf _vsnprintf

//   this one doesn't work to measure strings
//   #    define vsnprintf(buf,len,format,args) _vsnprintf_s(buf,len,(len)/sizeof(TEXTCHAR),format,args)
//   this one doesn't macro well, and doesnt' measure strings
//  (SUCCEEDED(StringCbVPrintf( buf, len, format, args ))?StrLen(buf):-1)

#      define snprintf portable_snprintf

//   this one gives deprication warnings
//   #    define snprintf _snprintf

//   this one doesn't work to measure strings
//   #    define snprintf(buf,len,format,...) _snprintf_s(buf,len,(len)/sizeof(TEXTCHAR),format,##__VA_ARGS__)
//   this one doesn't macro well, and doesnt' measure strings
//   (SUCCEEDED(StringCbPrintf( buf, len, format,##__VA_ARGS__ ))?StrLen(buf):-1)

// make sure this is off, cause we really don't, and have to include the following
#      undef HAVE_SNPRINTF
#      define PREFER_PORTABLE_SNPRINTF // define this anyhow so we can avoid name collisions
#      ifdef SACK_CORE_BUILD
#        include <../src/snprintf_2.2/snprintf.h>
#      else
#        include <snprintf-2.2/snprintf.h>
#      endif // SACK_CORE_BUILD
#    else // SUFFER_WITH_WARNININGS
#      if defined( _UNICODE ) && !defined( NO_UNICODE_C )
#        define snprintf _snwprintf
#        define vsnprintf _vsnwprintf
#      else
#        define snprintf _snprintf
#        define vsnprintf _vsnprintf
#      endif
#      if defined( _UNICODE )
#        define tnprintf _snwprintf
#        define vtnprintf _vsnwprintf
#      else
#        define tnprintf _snprintf
#        define vtnprintf _vsnprintf
#      endif
#    define snwprintf _snwprintf
#    endif// suffer_with_warnings

#    if defined( _UNICODE ) && !defined( NO_UNICODE_C )
#    define sscanf swscanf_s
#    else
#    define sscanf sscanf_s
#    endif

#  endif // _MSC_VER

#ifdef  __GNUC__
#      if defined( _UNICODE )
#        define tnprintf _snwprintf
#        define vtnprintf _vsnwprintf
#        ifndef NO_UNICODE_C && !defined( NO_UNICODE_C )
#           define snprintf   snwprintf
#           define vsnprintf  vsnwprintf
#           define sscanf     swscanf
#        else
#        endif
#      else
#        define tnprintf snprintf
#        define vtnprintf vsnprintf
//#        define snprintf snprintf
//#        define vsnprintf vsnprintf
#      endif
#        define snwprintf  _snwprintf

#endif // __GNUC__


#ifdef __WATCOMC__
#      if defined( _UNICODE )
#        define tnprintf  _snwprintf
#        define vtnprintf _vsnwprintf
#        ifndef NO_UNICODE_C && !defined( NO_UNICODE_C )
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

#endif // __WATCOMC__

#endif
