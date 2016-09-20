#ifndef LISTHIDS_H
#define LISTHIDS_H
#include <stdhdrs.h>

#ifdef SACKHIDLIST_SOURCE
#define SACKHIDLIST_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define SACKHIDLIST_PROC(type,name) IMPORT_METHOD type CPROC name
#endif


#define VERSION_LISTHIDS 0x00020000

#ifdef __cplusplus
extern "C"{
#endif

//#include <windows.h>

typedef struct
{
	TEXTSTR lpTechnology;
	TEXTSTR lpHid;
	TEXTSTR lpClass;
	TEXTSTR lpClassGuid;
	
}LISTHIDS_HIDINFO;


typedef LOGICAL (CPROC* ListHidsCallback)( uintptr_t psv, LISTHIDS_HIDINFO* lpHidInfo );

SACKHIDLIST_PROC( LOGICAL, ListHids )( ListHidsCallback lpCallback, uintptr_t psv );

#ifdef __cplusplus
}
#endif

#elif VERSION_LISTHIDS!=0x00020000
#error You have included two LISTHIDS.H with different version numbers
#endif