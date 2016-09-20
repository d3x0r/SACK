
#ifndef TASK_STRUCTURES_DEFINED
#define TASK_STRUCTURES_DEFINED

struct task_security_module
{
	CTEXTSTR name;
	PLIST tokens;
};

typedef struct task_tag
{
   DeclareLink( struct task_tag );
	struct {
		BIT_FIELD bConfirm : 1;
		BIT_FIELD bPassword : 1;
		//BIT_FIELD bImage : 1;
		//BIT_FIELD bUpdated : 1; // key modified...
		BIT_FIELD bLaunchAt : 1;
		BIT_FIELD bLaunchWhenCallerUp : 1;
		BIT_FIELD bAutoLaunch : 1;
		BIT_FIELD bExclusive : 1;
		BIT_FIELD bOneLaunch : 1;  // not same as exclusive, can launch in parallel with other things.
		BIT_FIELD bOneLaunchClickStop : 1; // clicking the same button will stop a one-launch process
		BIT_FIELD bRestart : 1; // if it stops, respawn it.
		BIT_FIELD bDestroy : 1; // set if destroyed, but launched...
		BIT_FIELD bButton : 1; // if it's created as a control instead of via common load
		BIT_FIELD bCaptureOutput : 1;
		BIT_FIELD bDisallowedRun : 1; // system mismatched
		BIT_FIELD bAllowedRun : 1;
		BIT_FIELD bStarting : 1; // still working on thinking about starting this...
		BIT_FIELD bBackground : 1;
		BIT_FIELD bHideCanvas : 1;
		BIT_FIELD bLaunchAtLeast : 1;
		BIT_FIELD bNoChangeResolution : 1;
		BIT_FIELD bWaitForTask : 1;
	} flags;
	uint32_t last_lauch_time;
   uint32_t launch_count;
   uint32_t launch_width, launch_height;

	TEXTCHAR pName[256];
	TEXTCHAR pTask[256], pPath[256];
	TEXTCHAR **pArgs;
	TEXTCHAR pShutdownTask[256], pShutdownPath[256];
	TEXTCHAR **pShutdownArgs;
	//CDATA color, textcolor;
	//char pImage[256];
	PLIST spawns;
	struct menu_button *button;
	CDATA highlight_normal_color;  // kept when button is loaded from button normal background color
	CDATA highlight_color;
	PLIST allowed_run_on;
	PLIST disallowed_run_on;
	PTHREAD waiting_thread;
	uintptr_t psvSecurityToken;
	PLIST security_modules;
} LOAD_TASK, *PLOAD_TASK;


PLOAD_TASK CPROC CreateTask( struct menu_button * );
void DestroyTask( PLOAD_TASK *ppTask );

//void CPROC RunATask( uintptr_t psv );
//void KillSpawnedPrograms( void );
int LaunchAutoTasks( int bCaller );



#endif
