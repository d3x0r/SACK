#include <stdio.h>


#define DEFAULT_PRELOAD_PRIORITY 69
#define DEADSTART_PRELOAD_PRIORITY 69


#if defined( __cplusplus) || defined( _WIN64 )

#define PRIORITY_PRELOAD(name,priority) static void name(void); \
   static class schedule_##name {   \
     public:schedule_##name() {    \
	name();\
	  }  \
	} do_schedul_##name;     \
	static void name(void)
#define MAGIC_PRIORITY_PRELOAD(name,priority) static void name(void); \
   static class schedule_##name {   \
	  public:schedule_##name() {  \
	name();  \
	  }  \
	} do_schedul_##name;     \
	static void name(void)
#define ATEXIT_PRIORITY(name,priority) static void name(void); \
   static class schedule_##name {   \
     public:schedule_##name() {    \
	name();\
	  }  \
	} do_schedul_##name;     \
	static void name(void)
#define PRIORITY_ATEXIT(name,priority) static void name(void); \
   static class shutdown_##name {   \
	public:shutdown_##name() {    \
	name();\
	/*name(); / * call on destructor of static object.*/ \
	  }  \
	} do_shutdown_##name;     \
	static void name(void)


#define PRELOAD(name) PRIORITY_PRELOAD(name,DEFAULT_PRELOAD_PRIORITY)
#define ATEXIT(name)      PRIORITY_ATEXIT(name,ATEXIT_PRIORITY_DEFAULT)
#define ROOT_ATEXIT(name) ATEXIT_PRIORITY(name,ATEXIT_PRIORITY_ROOT)

//------------------------------------------------------------------------------------
// Win32 Watcom
//------------------------------------------------------------------------------------
#elif defined( __WATCOMC__ )
#pragma off (check_stack)
/* code taken from openwatcom/bld/watcom/h/rtinit.h */
typedef unsigned char   __type_rtp;
typedef unsigned short  __type_pad;
typedef void(*__type_rtn ) ( void );
#ifdef __cplusplus
#pragma pack(1)
#else
#pragma pack(1)
#endif
struct rt_init // structure placed in XI/YI segment
{
    __type_rtp  rtn_type; // - near=0/far=1 routine indication
                          //   also used when walking table to flag
                          //   completed entries
    __type_rtp  priority; // - priority (0-highest 255-lowest)
    __type_rtn  rtn;      // - routine
};
#pragma pack()
/* end code taken from openwatcom/bld/watcom/h/rtinit.h */


/* in the main program is a routine which is referenced in dynamic mode...
 need to redo this macro in the case of static linking...

 */

//------------------------------------------------------------------------------------
//  WIN32 basic methods... on wait
//------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------
// watcom
//------------------------------------------------------------------------------------
//void RegisterStartupProc( void (*proc)(void) );


#define PRIORITY_PRELOAD(name,priority) static void schedule_##name(void); static void name(void); \
	static struct rt_init __based(__segname("XI")) name##_ctor_label={0,(DEADSTART_PRELOAD_PRIORITY-1),schedule_##name}; \
	static void schedule_##name(void) {                 \
	name();\
	}                                       \
	static void name(void)
#define ATEXIT_PRIORITY(name,priority) static void schedule_exit_##name(void); static void name(void); \
	static struct rt_init __based(__segname("XI")) name##_dtor_label={0,69,schedule_exit_##name}; \
	static void schedule_exit_##name(void) {                                              \
	name();\
	}                                       \
	static void name(void)

// syslog runs preload at priority 65
// message service runs preload priority 66
// deadstart itself tries to run at priority 70 (after all others have registered)
#define PRELOAD(name) PRIORITY_PRELOAD(name,DEFAULT_PRELOAD_PRIORITY)


// this is a special case macro used in client.c
// perhaps all PRIORITY_ATEXIT routines should use this
// this enables cleaning up things that require threads to be
// active under windows... (message disconnect)
// however this routine is only triggered in windows by calling
// BAG_Exit(nn) which is aliased to replace exit(n) automatically

#define PRIORITY_ATEXIT(name,priority) ATEXIT_PRIORITY( name,priority)
/*
static void name(void); static void name##_x_(void);\
	static struct rt_init __based(__segname("YI")) name##_dtor_label={0,priority,name##_x_}; \
	static void name##_x_(void) { char myname[256];myname[0]=*(CTEXTSTR)&name##_dtor_label;GetModuleFileName(NULL,myname,sizeof(myname));name(); } \
	static void name(void)
  */
#define ROOT_ATEXIT(name) ATEXIT_PRIORITY(name,ATEXIT_PRIORITY_ROOT)
#define ATEXIT(name)      PRIORITY_ATEXIT(name,ATEXIT_PRIORITY_DEFAULT)
// if priority_atexit is used with priority 0 - the proc is scheduled into
// atexit, and exit() is then invoked.
//#define PRIORITY_ATEXIT(name,priority) ATEXIT_PRIORITY(name,priority )

#endif


PRELOAD( test )
{
   printf( "test success\n" );
}

__declspec(dllimport) int library( void );

int main( void )
{
   library();
	printf( "done." );
   return 0;
}
