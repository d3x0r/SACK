https://godbolt.org/z/WhE3qEoYW

#include <stdio.h>

#if !defined( __NO_THREAD_LOCAL__ ) && ( defined( _MSC_VER ) || defined( __WATCOMC__ ) )
#  define HAS_TLS 1
#  ifdef __cplusplus
#    define DeclareThreadLocal static thread_local
#    define DeclareThreadVar  thread_local
#  else
#    define DeclareThreadLocal static __declspec(thread)
#    define DeclareThreadVar __declspec(thread)
#  endif
#elif !defined( __NO_THREAD_LOCAL__ ) && ( defined( __GNUC__ ) || defined( __MAC__ ) )
#    define HAS_TLS 1
#    ifdef __cplusplus
#      define DeclareThreadLocal static thread_local
#      define DeclareThreadVar thread_local
#    else
#    define DeclareThreadLocal static __thread
#    define DeclareThreadVar __thread
#  endif
#else
// if no HAS_TLS
#  define DeclareThreadLocal static
#  define DeclareThreadVar
#endif


struct my_thread_info {
	int pThread;
	int nThread;
};
DeclareThreadLocal  struct my_thread_info _MyThreadInfo;

int f( void ) {
    if( !_MyThreadInfo.pThread )
        _MyThreadInfo.pThread = 1;
	int a = _MyThreadInfo.pThread;
	int b = _MyThreadInfo.pThread;
	int c = _MyThreadInfo.pThread;
	printf( "Use vars %d %d %d\n", a, b, c );
    return _MyThreadInfo.pThread;

}


int main( void ) {
    f();
}