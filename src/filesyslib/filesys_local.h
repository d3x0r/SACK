
static struct winfile_local_tag {
	CRITICALSECTION cs_files;
	PLIST files;
	PLIST groups;
	PLIST handles;
	PLIST file_system_interface;
	struct file_system_interface *default_file_system_interface;
	LOGICAL have_default;
	struct {
		BIT_FIELD bLogOpenClose : 1;
		BIT_FIELD bInitialized : 1;
	} flags;
	TEXTSTR data_file_root;
	TEXTSTR producer;
	TEXTSTR application;

} *winfile_local;

#define l (*winfile_local)
