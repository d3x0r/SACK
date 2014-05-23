#ifndef _PORTABLE_SNPRINTF_H_
#define _PORTABLE_SNPRINTF_H_

#ifdef __cplusplus
namespace sack {
	namespace compat {
		namespace msvc_snprintf {
#endif

#ifdef SACK_BAG_EXPORTS
#define SNPRINTF_EXTERN EXPORT_METHOD
#else
#define SNPRINTF_EXTERN IMPORT_METHOD
#endif

#define PORTABLE_SNPRINTF_VERSION_MAJOR 2
#define PORTABLE_SNPRINTF_VERSION_MINOR 2

#if !defined(PREFER_PORTABLE_SNPRINTF)
SNPRINTF_EXTERN int snprintf(char *, size_t, const char *, /*args*/ ...);
SNPRINTF_EXTERN int vsnprintf(char *, size_t, const char *, va_list);
#endif

#if defined(PREFER_PORTABLE_SNPRINTF)
SNPRINTF_EXTERN int portable_snprintf(char *str, size_t str_m, const char *fmt, /*args*/ ...);
SNPRINTF_EXTERN int portable_vsnprintf(char *str, size_t str_m, const char *fmt, va_list ap);
#define snprintf  portable_snprintf
#define vsnprintf portable_vsnprintf
#endif

SNPRINTF_EXTERN int asprintf  (char **ptr, const char *fmt, /*args*/ ...);
SNPRINTF_EXTERN int vasprintf (char **ptr, const char *fmt, va_list ap);
SNPRINTF_EXTERN int asnprintf (char **ptr, size_t str_m, const char *fmt, /*args*/ ...);
SNPRINTF_EXTERN int vasnprintf(char **ptr, size_t str_m, const char *fmt, va_list ap);

#ifdef __cplusplus
		}//namespace msvc_snprintf {
	}//namespace compat {
}//namespace sack {
using namespace sack::compat::msvc_snprintf;
#endif

#endif
