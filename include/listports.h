#ifndef LISTPORTS_H
#define LISTPORTS_H
#include <stdhdrs.h>

#ifdef SACKCOMMLIST_SOURCE
#define SACKCOMMLIST_PROC(type,name) EXPORT_METHOD type CPROC name
#else
#define SACKCOMMLIST_PROC(type,name) IMPORT_METHOD type CPROC name
#endif


#define VERSION_LISTPORTS 0x00020000

#ifdef __cplusplus
extern "C"{
#endif

//#include <windows.h>

typedef struct
{
	TEXTSTR lpPortName;     /* "COM1", etc. */
	CTEXTSTR lpFriendlyName; /* Suitable to describe the port, as for  */
							/* instance "Infrared serial port (COM4)" */
	CTEXTSTR lpTechnology;   /* "BIOS","INFRARED","USB", etc.          */

}LISTPORTS_PORTINFO;


typedef LOGICAL (CPROC* ListPortsCallback)( PTRSZVAL psv, LISTPORTS_PORTINFO* lpPortInfo );
/* User provided callback funtion that receives the information on each
 * serial port available.
 * The strings provided on the LISTPORTS_INFO are not to be referenced after
 * the callback returns; instead make copies of them for later use.
 * If the callback returns FALSE, port enumeration is aborted.
 */
SACKCOMMLIST_PROC( LOGICAL, ListPorts )( ListPortsCallback lpCallback, PTRSZVAL psv );
/* Lists serial ports available on the system, passing the information on
 * each port on succesive calls to lpCallback.
 * lpCallbackValue, treated opaquely by ListPorts(), is intended to carry
 * information internal to the callback routine.
 * Returns TRUE if succesful, otherwise error code can be retrieved via
 * GetLastError().
 */

#ifdef __cplusplus
}
#endif

#elif VERSION_LISTPORTS!=0x00020000
#error You have included two LISTPORTS.H with different version numbers
#endif