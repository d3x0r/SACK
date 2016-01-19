/**
 *	@file    osdep.c
 *	@version e6a3d9f (HEAD, tag: MATRIXSSL-3-7-1-OPEN, origin/master, origin/HEAD, master)
 *
 *	WIN32 platform PScore .
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

#include "../coreApi.h"

#ifdef WIN32 
#include <windows.h>
#include <wincrypt.h>
/******************************************************************************/
/*
    TIME FUNCTIONS
*/
/******************************************************************************/
static LARGE_INTEGER	hiresStart; /* zero-time */
static LARGE_INTEGER	hiresFreq; /* tics per second */
/*
    Module open and close
*/
int osdepTimeOpen(void)
{
	if (QueryPerformanceFrequency(&hiresFreq) != PS_TRUE) {
		return PS_FAILURE;
	}
	if (QueryPerformanceCounter(&hiresStart) != PS_TRUE) {
		return PS_FAILURE;
	}
	return PS_SUCCESS;
}

void osdepTimeClose(void)
{
}

/* FILETIME of Jan 1 1970 00:00:00. */
static const unsigned __int64 epoch = ((unsigned __int64) 116444736000000000ULL);

/*
 * timezone information is stored outside the kernel so tzp isn't used anymore.
 *
 * Note: this function is not for Win32 high precision timing purpose. See
 * elapsed_time().
 */
int
gettimeofday(psTime_t * tp, struct timezone * tzp)
{
    FILETIME    file_time;
    SYSTEMTIME  system_time;
    ULARGE_INTEGER ularge;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    ularge.LowPart = file_time.dwLowDateTime;
    ularge.HighPart = file_time.dwHighDateTime;

    tp->tv_sec = (long) ((ularge.QuadPart - epoch) / 10000000L);
    tp->tv_usec = (long) (system_time.wMilliseconds * 1000);

    return 0;
}

/*
    PScore Public API implementations
*/
int32 psGetTime(psTime_t *t, void *userPtr)
{
	psTime_t	lt;
    __int64		diff;
	int32			d;
	if (t == NULL) {
		gettimeofday( &lt, NULL );
		//QueryPerformanceCounter(&lt);
		return lt.tv_sec;
	}

	gettimeofday( t, NULL );
	//QueryPerformanceCounter(t);
	//diff = t->QuadPart/* - hiresStart.QuadPart*/;
	//d = (int32)((diff * 1000) / hiresFreq.QuadPart);
	return t->tv_sec;
}

int32 psDiffMsecs(psTime_t then, psTime_t now, void *userPtr) 
{
    __int64	diff;

	diff = now.tv_sec - then.tv_sec;
	return (int32)((diff*1000));
}

int32 psCompareTime(psTime_t a, psTime_t b, void *userPtr)
{
	if (a.tv_sec <= b.tv_sec) {
		return 1;
	}
	return 0;
}


#ifdef USE_MULTITHREADING
/******************************************************************************/
/*
    MUTEX FUNCTIONS
*/
/******************************************************************************/
/*
    Module open and close
*/
int osdepMutexOpen(void)
{
	return PS_SUCCESS;
}

int osdepMutexClose(void)
{
	return PS_SUCCESS;
}

/*
    PScore Public API implementations
*/
int32 psCreateMutex(psMutex_t *mutex)
{
	InitializeCriticalSection(mutex);
	return PS_SUCCESS;
}

int32 psLockMutex(psMutex_t *mutex)
{
	EnterCriticalSection(mutex);
	return PS_SUCCESS;
}

int32 psUnlockMutex(psMutex_t *mutex)
{
	LeaveCriticalSection(mutex);
	return PS_SUCCESS;
}

void psDestroyMutex(psMutex_t *mutex)
{
	DeleteCriticalSection(mutex);
}
#endif /* USE_MULTITHREADING */
/******************************************************************************/


/******************************************************************************/
/*
    ENTROPY FUNCTIONS
*/
/******************************************************************************/
#ifdef MINGW_SUX
#define PROV_RSA_FULL           1
#define PROV_RSA_SIG            2
#define PROV_DSS                3
#define PROV_FORTEZZA           4
#define PROV_MS_EXCHANGE        5
#define PROV_SSL                6
#define PROV_RSA_SCHANNEL       12
#define PROV_DSS_DH             13
#define PROV_EC_ECDSA_SIG       14
#define PROV_EC_ECNRA_SIG       15
#define PROV_EC_ECDSA_FULL      16
#define PROV_EC_ECNRA_FULL      17
#define PROV_DH_SCHANNEL        18
#define PROV_SPYRUS_LYNKS       20
#define PROV_RNG                21
#define PROV_INTEL_SEC          22

#define CRYPT_VERIFYCONTEXT     0xF0000000
#define CRYPT_NEWKEYSET         0x00000008
#define CRYPT_DELETEKEYSET      0x00000010
#define CRYPT_MACHINE_KEYSET    0x00000020
#define CRYPT_SILENT            0x00000040

typedef ULONG_PTR HCRYPTPROV;
#endif
static HCRYPTPROV		hProv;	/* Crypto context for random bytes */

int osdepEntropyOpen(void)
{
	if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, 
			CRYPT_VERIFYCONTEXT))  {
		return PS_FAILURE;
	}
	return PS_SUCCESS;
}

void osdepEntropyClose(void)
{
	CryptReleaseContext(hProv, 0);
}

int32 psGetEntropy(unsigned char *bytes, uint32 size, void *userPtr)
{
	if (CryptGenRandom(hProv, size, bytes)) {
		return size;
	}
	return PS_FAILURE;
}


/******************************************************************************/
/*
    TRACE FUNCTIONS
*/
/******************************************************************************/
int osdepTraceOpen(void)
{
	return PS_SUCCESS;
}

void osdepTraceClose(void)
{
}

void _psTrace(char *msg)
{
    printf(msg);
}

/* message should contain one %s, unless value is NULL */
void _psTraceStr(char *message, char *value)
{
    if (value) {
        printf(message, value);
    } else {
        printf(message);
    }
}

/* message should contain one %d */
void _psTraceInt(char *message, int32 value)
{
    printf(message, value);
}

/* message should contain one %p */
void _psTracePtr( char *message, void *value)
{
    printf(message, value);
}


#ifdef HALT_ON_PS_ERROR
/******************************************************************************/
/*
    system halt on psError when built HALT_ON_PS_ERROR 
*/
void osdepBreak(void)
{
     DebugBreak();
}
#endif /* HALT_ON_PS_ERROR */


#ifdef MATRIX_USE_FILE_SYSTEM
/******************************************************************************/
/*
    FILE ACCESS FUNCTION
*/
/******************************************************************************/
/*
    Memory info:
    Caller must free 'buf' parameter on success
    Callers do not need to free buf on function failure
*/
int32 psGetFileBuf(psPool_t *pool, const char *fileName, unsigned char **buf,
                int32 *bufLen)
{
	DWORD	dwAttributes;
	HANDLE	hFile;
	int32	size;
	DWORD	tmp = 0;

	*bufLen = 0;
	*buf = NULL;

	dwAttributes = GetFileAttributesA(fileName);
	if (dwAttributes != 0xFFFFFFFF && dwAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		psTraceStrCore("Unable to find %s\n", (char*)fileName);
        return PS_PLATFORM_FAIL;
	}

	if ((hFile = CreateFileA(fileName, GENERIC_READ,
			FILE_SHARE_READ && FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE) {
		psTraceStrCore("Unable to open %s\n", (char*)fileName);
        return PS_PLATFORM_FAIL;
	}
	
/*
 *	 Get the file size.
 */
	size = GetFileSize(hFile, NULL);

	*buf = psMalloc(pool, size + 1);
	if (*buf == NULL) {
		CloseHandle(hFile);
		return PS_MEM_FAIL;
	}
	memset(*buf, 0x0, size + 1);
	
	while (*bufLen < size) { 
		if (ReadFile(hFile, *buf + *bufLen,
				(size-*bufLen > 512 ? 512 : size-*bufLen), &tmp, NULL)
				== FALSE) {
			psFree(*buf, pool);
			psTraceStrCore("Unable to read %s\n", (char*)fileName);
			CloseHandle(hFile);
			return PS_PLATFORM_FAIL;
		}
		*bufLen += (int32)tmp;
	}

	CloseHandle(hFile);
	return PS_SUCCESS;
}
#endif /* MATRIX_USE_FILE_SYSTEM */


/******************************************************************************/
#endif /* WIN32 */
/******************************************************************************/





