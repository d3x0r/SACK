
#ifdef _WIN32
#define PATHCHAR "\\"
#else
#define PATHCHAR "/"
#endif

struct file_system_mounted_interface 
{
	struct file_system_mounted_interface* nextLayer;
	//struct file_system_mounted_interface* sideLayer; // mount-as... 
	struct file_system_mounted_interface* parent;
	//DeclareLink( struct file_system_mounted_interface );
	const char *name;
	int priority;
	uintptr_t psvInstance;
	struct file_system_interface *fsi;
	LOGICAL writeable;
};

#if !defined( WINFILE_COMMON_SOURCE ) && defined( __STATIC_GLOBALS__ )
extern
#endif
 struct winfile_local_tag {
	CRITICALSECTION cs_files;
	PLIST files;
	PLIST directories;
	PLIST groups;
	PLIST handles;
	PLIST file_system_interface;
	struct file_system_interface *default_file_system_interface;
	struct file_system_mounted_interface *_mounted_file_systems;
	struct file_system_mounted_interface *last_find_mount;
	struct file_system_mounted_interface *_default_mount;

	LOGICAL have_default;
	struct {
		BIT_FIELD bLogOpenClose : 1;
		BIT_FIELD bInitialized : 1;
		BIT_FIELD bDeallocateClosedFiles : 1;
	} flags;
	TEXTSTR local_data_file_root;
	TEXTSTR data_file_root;
	TEXTSTR producer;
	TEXTSTR application;

 }
#ifdef __STATIC_GLOBALS__
winfile_local__;
#endif
;

#if HAS_TLS
struct filesys_thread_mount_info {
	struct file_system_mounted_interface* default_mount;
	struct file_system_mounted_interface** _mounted_file_systems;
#define mounted_file_systems _mounted_file_systems[0]
	struct file_system_mounted_interface* thread_local_mounted_file_systems;
	char* cwd;
};
#if !defined( WINFILE_COMMON_SOURCE )
extern
#endif
	DeclareThreadVar  struct filesys_thread_mount_info _fileSysThreadInfo;
#  define FileSysThreadInfo (_fileSysThreadInfo)

#else
#error "Please get a better platform target...."
//#  define fileSysThreadInfo (*_fileSysThreadInfo)
#endif


#ifndef WINFILE_COMMON_SOURCE
extern
#endif
	struct winfile_local_tag *winfile_local
#if defined( __STATIC_GLOBALS__ ) && defined( WINFILE_COMMON_SOURCE )
		   = &winfile_local__
#endif
	;


//#define l (*winfile_local)
