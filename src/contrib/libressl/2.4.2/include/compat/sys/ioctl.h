/*
 * Public domain
 * sys/ioctl.h compatibility shim
 */

#if !defined( _WIN32 ) && !defined( _PNACL )
#include_next <sys/ioctl.h>
#else
#include <win32netcompat.h>
#define ioctl(fd, type, arg) ioctlsocket(fd, type, arg)
#endif
