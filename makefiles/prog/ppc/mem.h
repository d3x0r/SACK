
#if defined __WATCOMC__
//# include "sharemem.h"
#else

# include "./types.h"

void CPROC *AllocateEx( size_t nSize DBG_PASS );
# define Allocate(s) AllocateEx(s DBG_SRC)
void CPROC ReleaseExx( void ** DBG_PASS );

# if defined( __WATCOMC__ ) || defined( _MSC_VER )
#  ifdef _DEBUG
#   define ReleaseEx(p,f,l ) ReleaseExx( (void**)&p,f,l )
#  else
#   define ReleaseEx(p ) ReleaseExx( (void**)&p )
#  endif
# else
#  define ReleaseEx(... ) ReleaseExx( (void**)&__VA_ARGS__ )
# endif
#define Release(p) ReleaseExx( (void**)&p DBG_SRC)

void DumpMemory( void );

uint32_t CPROC LockedExchange( uint32_t *p, uint32_t val );

void CPROC MemSet( POINTER p, uint32_t v, size_t n);
void CPROC MemCpy( POINTER p, POINTER p2, size_t n);


void DisableMemoryValidate( int bDisable );

char CPROC *StrDupEx( const char *original DBG_PASS );
#define StrDup(o) StrDupEx(o DBG_SRC )

#endif
