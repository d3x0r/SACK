/**
 *	@file    osdep.h
 *	@version e6a3d9f (HEAD, tag: MATRIXSSL-3-7-1-OPEN, origin/master, origin/HEAD, master)
 *
 *	Operating System and Hardware Abstraction Layer.
 */
/*
 *	Copyright (c) 2013-2014 INSIDE Secure Corporation
 *	Copyright (c) PeerSec Networks, 2002-2011
 *	All Rights Reserved
 *
 *	The latest version of this code is available at http://www.matrixssl.org
 *
 *	This software is open source; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This General Public License does NOT permit incorporating this software 
 *	into proprietary programs.  If you are unable to comply with the GPL, a 
 *	commercial license for this software may be purchased from INSIDE at
 *	http://www.insidesecure.com/eng/Company/Locations
 *	
 *	This program is distributed in WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	See the GNU General Public License for more details.
 *	
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *	http://www.gnu.org/copyleft/gpl.html
 */
/******************************************************************************/

#ifndef _h_PS_PLATFORM
#define _h_PS_PLATFORM

/******************************************************************************/
/*
	Platform detection based on compiler settings
	@see http://sourceforge.net/p/predef/wiki/Home/
*/
/* Determine the operating system (if any) */
#if defined(__linux__) /* Linux and Android */
 #define POSIX
 #define LINUX
 #define MATRIX_USE_FILE_SYSTEM
#elif defined(__APPLE__) && defined(__MACH__) /* Mac OS X */
 #define POSIX
 #define OSX
 #define MATRIX_USE_FILE_SYSTEM
#elif defined(_WIN32) /* Windows */
 //#define WIN32
 #define HAVE_NATIVE_INT64 /* Windows will always have this */
 #define MATRIX_USE_FILE_SYSTEM
#endif
/* For others such as FREERTOS, define in build system */

/* Determine which assembly language optimizations we can use */
#if defined(__GNUC__) || defined(__clang__) /* Only supporting gcc-like */
#if defined(__x86_64__)
 #define PSTM_X86_64
 #define PSTM_64BIT /* Supported by architecture */
#elif defined(__i386__)
#ifndef MINGW_SUX
 #define PSTM_X86
#endif
#elif defined(__arm__)
 #define PSTM_ARM
 //__aarch64__ /* 64 bit arm */
 //__thumb__ /* Thumb mode */
#elif defined(__mips__)
 #define PSTM_MIPS
#else
 #error architecture undefined.
#endif
#endif /* GNUC/CLANG */

/* Try to determine if the compiler/platform supports 64 bit integer ops */
#if defined(__SIZEOF_LONG_LONG__)
 #define HAVE_NATIVE_INT64 /* Supported by compiler */
#endif

/* Detect endian */
#if defined __LITTLE_ENDIAN__ || defined __i386__ || defined __x86_64__ || \
	defined _M_X64 || defined _M_IX86 || \
	defined __ARMEL__ || defined __MIPSEL__
 #define __ORDER_LITTLE_ENDIAN__ 1234
 #define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
#endif
#ifdef __BYTE_ORDER__        /* Newer GCC and LLVM */
 #if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  #define ENDIAN_LITTLE
 #else
  #define ENDIAN_BIG
 #endif
 #ifdef PSTM_64BIT
  #define ENDIAN_64BITWORD
 #else
  #define ENDIAN_32BITWORD
 #endif
#else
 #if (defined(_MSC_VER) && defined(WIN32)) || \
   (defined(__GNUC__) && (defined(__DJGPP__) || defined(__CYGWIN__) || \
   defined(__MINGW32__) || defined(__i386__)))
  #define ENDIAN_LITTLE
  #define ENDIAN_32BITWORD
 #else
  #warning "Cannot determine endianness, using neutral"
 #endif
/* #define ENDIAN_LITTLE */
/* #define ENDIAN_BIG */

/* #define ENDIAN_32BITWORD */
/* #define ENDIAN_64BITWORD */
#endif

#if (defined(ENDIAN_BIG) || defined(ENDIAN_LITTLE)) && \
!(defined(ENDIAN_32BITWORD) || defined(ENDIAN_64BITWORD))
#error You must specify a word size as well as endianness
#endif

#if !(defined(ENDIAN_BIG) || defined(ENDIAN_LITTLE))
#define ENDIAN_NEUTRAL
#endif

/******************************************************************************/
/*
    APIs that must be implemented on every platform
*/
extern int		osdepTraceOpen(void);
extern void		osdepTraceClose(void);
extern int		osdepTimeOpen(void);
extern void		osdepTimeClose(void);
extern int		osdepEntropyOpen(void);
extern void		osdepEntropyClose(void);
#ifdef HALT_ON_PS_ERROR
	extern void  osdepBreak(void);
#endif
#ifndef min
	#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif /* min */

/******************************************************************************/
/*
    If the Makefile specifies that MatrixSSL does not currently have
    a layer for the given OS, or the port is to "bare metal" hardware,
    do basic defines here and include externally provided file "matrixos.h".
    In addition, if building for such a platform, a C file defining the above
    functions must be linked with the final executable.
*/
#ifdef PS_UNSUPPORTED_OS
    #define PSPUBLIC extern
    #include "matrixos.h"
#else
/******************************************************************************/
/*
    Supported Platforms below. The implementations of the apis are in
    platform specific directories, such as core/POSIX and core/ECOS

    POSIX define is used for Linux and Mac OS X
*/
#include <stdio.h>
	
#ifndef POSIX
	#if defined(LINUX) || defined(OSX)
	#define POSIX
	#endif
#endif

#ifdef POSIX
	#include <stdint.h>
	typedef int32_t int32;
	typedef uint32_t uint32;
	typedef int16_t int16;
	typedef uint16_t uint16;
	typedef uint8_t uint8;
	#ifdef HAVE_NATIVE_INT64 
	typedef int64_t int64;
	typedef uint64_t uint64;
	#endif
#endif /* POSIX */
#ifdef WIN32
	#include <windows.h>
	#define strcasecmp lstrcmpiA
	#define snprintf _snprintf
	typedef signed long int32;
	typedef unsigned long uint32;
	typedef signed short int16;
	typedef unsigned short uint16;
	typedef unsigned char uint8;
	#ifdef HAVE_NATIVE_INT64 
	typedef unsigned long long	uint64;
	typedef signed long long	int64;
	#endif
#endif /* WIN32 */

/******************************************************************************/
/*
	Hardware Abstraction Layer
*/
#define halAlert()

/******************************************************************************/
/*
	OS-specific psTime_t types
	
	Make psTime_t an opaque time value.
*/
#ifdef WIN32
	typedef LARGE_INTEGER psTime_t;
#endif
#ifdef VXWORKS
	typedef struct {
		long sec;
		long usec;
	} psTime_t;
#endif

/* #define USE_HIGHRES_TIME */
#ifdef POSIX
	#ifndef USE_HIGHRES_TIME
		#include <sys/time.h>
		#include <time.h>
		typedef struct timeval psTime_t;
	#else
		#if defined(__APPLE__) || defined(__tile__)
			typedef uint64_t psTime_t;
		#else
			typedef struct timespec psTime_t;
		#endif
		extern int64_t psDiffUsecs(psTime_t then, psTime_t now);
	#endif
#endif

/******************************************************************************/
/*
	Export or import functions for Windows DLLs	
*/
#ifdef WIN32
	#ifndef _USRDLL
	#define PSPUBLIC extern __declspec(dllimport)
	#else
	#define PSPUBLIC extern __declspec(dllexport)
	#endif
#else
	#define PSPUBLIC extern
#endif /* !WIN */

/******************************************************************************/
/*
	Raw trace and error
*/
PSPUBLIC void _psTrace(char *msg);
PSPUBLIC void _psTraceInt(char *msg, int32 val);
PSPUBLIC void _psTraceStr(char *msg, char *val);
PSPUBLIC void _psTracePtr(char *message, void *value);
PSPUBLIC void psTraceBytes(char *tag, unsigned char *p, int l);

PSPUBLIC void _psError(char *msg);
PSPUBLIC void _psErrorInt(char *msg, int32 val);
PSPUBLIC void _psErrorStr(char *msg, char *val);

/******************************************************************************/
/*
	Core trace
*/
#ifndef USE_CORE_TRACE
#define psTraceCore(x) 
#define psTraceStrCore(x, y) 
#define psTraceIntCore(x, y)
#define psTracePtrCore(x, y)
#else
#define psTraceCore(x) _psTrace(x)
#define psTraceStrCore(x, y) _psTraceStr(x, y)
#define psTraceIntCore(x, y) _psTraceInt(x, y)
#define psTracePtrCore(x, y) _psTracePtr(x, y)
#endif /* USE_CORE_TRACE */

/******************************************************************************/
/*
	HALT_ON_PS_ERROR define at compile-time determines whether to halt on
	psAssert and psError calls
*/ 
#define psAssert(C)  if (C) ; else \
{halAlert();_psTraceStr("psAssert %s", __FILE__);_psTraceInt(":%d ", __LINE__);\
_psError(#C);} 

#define psError(a) \
 halAlert();_psTraceStr("psError %s", __FILE__);_psTraceInt(":%d ", __LINE__); \
 _psError(a);
 
#define psErrorStr(a,b) \
 halAlert();_psTraceStr("psError %s", __FILE__);_psTraceInt(":%d ", __LINE__); \
 _psErrorStr(a,b)
 
#define psErrorInt(a,b) \
 halAlert();_psTraceStr("psError %s", __FILE__);_psTraceInt(":%d ", __LINE__); \
 _psErrorInt(a,b)

/******************************************************************************/
/*
	OS specific file system apis
*/
#ifdef MATRIX_USE_FILE_SYSTEM
	#ifdef POSIX
	#include <sys/stat.h>
	#endif /* POSIX */
#endif /* MATRIX_USE_FILE_SYSTEM */

#ifdef USE_MULTITHREADING
/******************************************************************************/
/*
	Defines to make library multithreading safe
*/
extern int32 osdepMutexOpen(void);
extern int32 osdepMutexClose(void);

#ifdef WIN32
	typedef CRITICAL_SECTION psMutex_t;
#elif POSIX
	#include <pthread.h>
	#include <string.h>
	typedef pthread_mutex_t psMutex_t;
#elif VXWORKS
	#include "semLib.h"
	typedef SEM_ID	psMutex_t; 
#else
	#error psMutex_t must be defined
#endif /* OS specific mutex */
#endif /* USE_MULTITHREADING */

/******************************************************************************/

#endif /* !PS_UNSUPPORTED_OS */
#endif /* _h_PS_PLATFORM */

