
struct file_system_mounted_interface 
{
	DeclareLink( struct file_system_mounted_interface );
	const char *name;
	int priority;
	PTRSZVAL psvInstance;
	struct file_system_interface *fsi;
	LOGICAL writeable;
};

#ifndef WINFILE_COMMON_SOURCE
extern
#endif
 struct winfile_local_tag {
	CRITICALSECTION cs_files;
	PLIST files;
	PLIST groups;
	PLIST handles;
	PLIST file_system_interface;
	struct file_system_interface *default_file_system_interface;
	struct file_system_mounted_interface *mounted_file_systems;
	struct file_system_mounted_interface *last_find_mount;
	struct file_system_mounted_interface *default_mount;

	LOGICAL have_default;
	struct {
		BIT_FIELD bLogOpenClose : 1;
		BIT_FIELD bInitialized : 1;
		BIT_FIELD bDeallocateClosedFiles : 1;
	} flags;
	TEXTSTR data_file_root;
	TEXTSTR producer;
	TEXTSTR application;

} *winfile_local;


//#define l (*winfile_local)
