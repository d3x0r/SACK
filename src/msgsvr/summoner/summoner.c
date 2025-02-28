#include <stdhdrs.h>
#include <deadstart.h> // ATEXIT
#include <idle.h>
#include <network.h>
#include <netservice.h>
#include <msgclient.h>
#include <configscript.h>
#ifdef WIN32
#include <systray.h>
#endif
#include <timers.h>
#include <sharemem.h>

#include "avatar_prot.h"

#include "summoner.h"

#ifdef BAG
#define PREFIX "bag."
#else
#define PREFIX
#endif

//#define DEBUG_WAIT_LIST

// messages for this service
// may include
//   IM_ALIVE
//   RU_READY
//   IM_READY
//   IS_READY // someone else I might be waiting on, but required parallel starting.
//enum {

enum summonerProtocol {
	SUMPROT_REQUIRE, // remote is requesting to be notified when task starts.
	SUMPROT_REQUIRMENU_STARTED, // remote is requesting to be notified when task starts.
	SUMPROT_REQUIRMENU_ENDED, // remote is requesting to be notified when task starts.
};

typedef struct required_task_info
{
	struct {
		// just a suggestion
		uint32_t bOptional : 1; // flags.bOptional is not a REQUIRED depenandcy
	} flags;
	char *name;
	char *address;
	struct my_task_info *task;
} *PREQUIRED_TASK, REQUIRED_TASK;

typedef struct my_task_info
{
	struct {
		uint32_t bSuspend : 1;
		uint32_t bStarted : 1;
		uint32_t bFailedSpawn : 1;
		uint32_t bStopped : 1; // task has been terminated - presumably during our exit-path... a
		uint32_t bTooFast : 1; // re-starts happen tooo fast... this stops bad tasks
		uint32_t bRestart : 1;
		uint32_t bTimeout : 1; // indicates that the ready_timeout is set (might be 0)
		uint32_t bRemote : 1; // task is meant for a remote box...
		uint32_t bScheduled : 1;
		uint32_t bScheduling : 1;
		uint32_t bWaiting : 1; // waiting for WHOAMI (so we can start launching tasks again)
		uint32_t bWaitForStart : 1; // waiting for IM_STARTING
		uint32_t bWaitForReady : 1; // waiting for IM_READY
		uint32_t bWaitAgain : 1;
		uint32_t bExclusive : 1; // when this is running, no other task may be running
	} flags;
	char *name;
	char *program;
	char **args;
	char *path;
	char *group; // name of the group this task is exclusive to...
	char *remote_address; // identifier of the remote system (IP? Name?)
	uint32_t ready_timeout;
	uint32_t last_launch_time;
	uint32_t min_launch_time;
	uint32_t launch_threshold_count; // more than these in min_launch_time
	uint32_t launch_count;
// list fo PMYTASK_INFOs which are required
// during configuration read time, until the end is processed,
// this is a list of char * task names - which will be converted into
// PMYTASK_INFOs - a sanity check after load will complain of circular
// dependancies.
	PLIST requires;
	PLIST required_by; // list of tasks which 'requires' this
	PLIST spawns; // list of tasks that should be run after this loadson
	DeclareLink( struct my_task_info );
} MYTASK_INFO, *PMYTASK_INFO;
//};

typedef struct network_state_tag
{
	uint32_t state;
	uint32_t cmd;
} NETSTATE, *PNETSTATE;

typedef struct local_tag
{
	struct {
		uint32_t something_bad_happened : 1;
		uint32_t bSpawnActive : 1; // a task has been spawned, and we are waiting for it to claim its ID
		uint32_t bSuspend : 1;
	} flags;
	char *configfile;
	PLIST tasks; // all known tasks.
	PMYTASK_INFO current_task;
	PMYTASK_INFO loaded; // put tasks in this list as loaded...
	PMYTASK_INFO schedule; // top-down list of sorted dependants (as well as could be)
	PMYTASK_INFO choked; // top-down list of sorted dependants (as well as could be)
	PMYTASK_INFO aware; // task has asked for WHOAMI and is 'aware' of its own id
	PMYTASK_INFO waiting_ready; // waiting for ready status...
	PMYTASK_INFO ready; // monitor with an alive probe?
	PCLIENT pcSpawner;
	uint16_t peer_port;
	uint16_t avatar_port;
	PTHREAD pLoadingThread;
} LOCAL;
static LOCAL l;

//--------------------------------------------------------------------------

static int CPROC SummonerEvents( PSERVICE_ROUTE SourceRouteID, uint32_t MsgID
										 , uint32_t *params, size_t param_length
										 , uint32_t *result, size_t *result_length )
{
	// receives messages, and forwards them to the network interface...
	// this task itself is able to spawn things(?)
	switch( MsgID )
	{
	case MSG_ServiceUnload:
		//(*result_length) = 0;
		return TRUE;
	case MSG_ServiceLoad:
		{
			MsgSrv_ReplyServiceLoad msg = ((MsgSrv_ReplyServiceLoad*)result)[0];
			msg.ServiceID = 0;
			msg.thread = GetMyThreadID();
			(*result_length) = sizeof( msg );
		}
		return TRUE;
	case MSG_WHOAMI:
		{
		//result[0] = 0; // task_id of some sort..
			if( l.current_task && l.flags.bSpawnActive )
			{
				strcpy( (char*)result, l.current_task->name );
				(*result_length) = strlen( l.current_task->name ) + 1;
				l.current_task->flags.bWaitAgain = 1;
				l.current_task->flags.bWaiting = 0;
				l.current_task->flags.bWaitForStart = 1;
				//lprintf( "Okay, task %s inquired for its ID", l.current_task->name );
				{
					PMYTASK_INFO task = l.current_task;
					RelinkThing( l.aware, task );
				}
				l.flags.bSpawnActive = 0;
			}
			else
			{
				// return no data success.
				(*result_length) = 0;
				return 1;
			}
		}
		return 1;
	case MSG_IM_STARTING:
		//printf( "%s is starting...\n", params );
		{
			PMYTASK_INFO task;
			for( task = l.aware; task; task = NextThing( task ) )
			//LIST_FORALL( l.tasks, idx, PMYTASK_INFO, task )
			{
				if( strcmp( (char*)params, task->name ) == 0 )
				{
					lprintf( "Marking %s as waiting for ready....", task->name );
				// all is well... continue starting tasks...

				// relink removes the node from its prior list and
				// puts it in this new list...
					RelinkThing( l.waiting_ready, task );
					task->flags.bWaitForStart = 0;
					task->flags.bWaitForReady = 1;
 					WakeThread( l.pLoadingThread );
					(*result_length) = 0; // return result MsgID
					return TRUE;
				}
			}
			if( !task )
			{
				lprintf( "Failed to find the task...\n" );
			}
			(*result_length) = 0;
			return FALSE;
		}
		//break;
	case MSG_IM_READY:
		{
			PMYTASK_INFO task;
			for( task = l.waiting_ready; task; task = NextThing( task ) )
			{
				if( strcmp( (char*)params, task->name ) == 0 )
				{
					lprintf( "%s is claiming it is ready...", (char*)params );
				// relink removes the node from its prior list and
				// puts it in this new list...
					RelinkThing( l.ready, task );
					task->flags.bWaitForReady = 0;
					task->flags.bStarted = 1;
					// at this point dependant tasks will be able to start...
					WakeThread( l.pLoadingThread );
					(*result_length) = 0; // allow result
					break;
				}
			}
			if( !task )
			{
				lprintf( "Failed to find task which claims it is starting : %s", (char*)params );
			}
		}
		break;
	case MSG_IM_ALIVE:
		// do something with the responce
		break;
	case MSG_RU_ALIVE:
		SendRoutedServerMessage( SourceRouteID, MSG_IM_ALIVE, params, param_length );
		break;
	case MSG_DIE:
		break;
	default:
		lprintf( "Received message %" _32f " from %" _32f " expecting %" _32f "... data follows..."
				 , MsgID, SourceRouteID, result_length?(*result_length):-1 );
		LogBinary( (uint8_t*)params, param_length );
		return FALSE;
	}
	return TRUE;
}

//--------------------------------------------------------------------------

static void CPROC ReadComplete( PCLIENT pc, POINTER buffer, size_t len )
{
	if( !buffer )
	{
		buffer = Allocate( 4096 );
	}
	ReadTCP( pc, buffer, 4096 );
}


//--------------------------------------------------------------------------

static int CPROC SpawnerConnect( uintptr_t psv, SOCKADDR *responder )
{
	if( l.pcSpawner )
		RemoveClient( l.pcSpawner );
	l.pcSpawner = OpenTCPClientAddrEx( responder, ReadComplete, NULL, NULL );
	return 1;
}

//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
static uintptr_t CPROC SetTaskReadyTimeout( uintptr_t psv, arg_list args )
{
	PARAM( args, uint64_t, timeout );
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	task->ready_timeout = timeout;
	return psv;
}

static uintptr_t CPROC SetTaskMinLaunchTime( uintptr_t psv, arg_list args )
{
	PARAM( args, uint64_t, timeout );
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	task->min_launch_time = timeout;
	return psv;
}

//--------------------------------------------------------------------------

static uintptr_t CPROC AddTaskSuggestion( uintptr_t psv, arg_list args )
{
	PARAM( args, char *, require );
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	PREQUIRED_TASK required_task_info = (PREQUIRED_TASK)Allocate( sizeof( REQUIRED_TASK ) );
	required_task_info->flags.bOptional = 1;
	required_task_info->name = StrDup( require );
	required_task_info->address = NULL;
	required_task_info->task = NULL;
	AddLink( &task->requires, required_task_info );
	return psv;
}

//--------------------------------------------------------------------------

static uintptr_t CPROC AddTaskRequirement( uintptr_t psv, arg_list args )
{
	PARAM( args, char *, require );
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	PREQUIRED_TASK required_task_info = (PREQUIRED_TASK)Allocate( sizeof( REQUIRED_TASK ) );
	required_task_info->flags.bOptional = 0;
	required_task_info->name = StrDup( require );
#ifdef DEBUG_WAIT_LIST
	lprintf( "Adding required %s to %s", require, task->name );
#endif
	required_task_info->address = NULL;
	required_task_info->task = NULL;
	AddLink( &task->requires, required_task_info );
	return psv;
}

//--------------------------------------------------------------------------

static uintptr_t CPROC LoadExternalService( uintptr_t psv, arg_list args )
{
	PARAM( args, char *, name );
// these are loaded at runtime, and
// forgotten...
	lprintf( "Loading external library %s", name );
	LoadPrivateFunction( name, NULL );
	return psv;
}

//--------------------------------------------------------------------------

static uintptr_t CPROC AddRemoteTaskRequirement( uintptr_t psv, arg_list args )
{
	PARAM( args, char *, require );
	PARAM( args, char *, address );
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	PREQUIRED_TASK required_task_info = (PREQUIRED_TASK)Allocate( sizeof( REQUIRED_TASK ) );
	required_task_info->name = StrDup( require );
	required_task_info->address = StrDup( address );
	required_task_info->task = NULL;
	AddLink( &task->requires, required_task_info );
	return psv;
}

//--------------------------------------------------------------------------

static uintptr_t CPROC SetTaskRestart( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bRestart );
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	task->flags.bRestart = bRestart;
	return psv;
}

//--------------------------------------------------------------------------

static uintptr_t CPROC SetTaskExclusive( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bExclusive );
	PARAM( args, CTEXTSTR, group );
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	task->flags.bExclusive = bExclusive;
	task->group = StrDup( group );
	return psv;
}

//--------------------------------------------------------------------------

static uintptr_t CPROC SetTaskProgram( uintptr_t psv, arg_list args )
{
	PARAM( args, char *, program );
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	task->program = StrDup( program );
	return psv;
}

//--------------------------------------------------------------------------

static uintptr_t CPROC SetTaskPath( uintptr_t psv, arg_list args )
{
	PARAM( args, char *, path );
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	task->path = StrDup( path );
	return psv;
}

//--------------------------------------------------------------------------

static void BuildArgs( char ***pppArgs, char *newargs )
{
	char  *p;
	char **pp;
	pp = *pppArgs;
	while( pp && pp[1] )
	{
		Release( pp[1] );
		pp++;
	}
	Release( *pppArgs );

	{
		int count = 0;
		int lastchar;
		lastchar = ' '; // auto continue spaces...
		//lprintf( "Got args: %s", newargs );
		p = newargs;
		while( p && p[0] )
		{
			if( !count ) // 1 at least...
				count++;
			if( lastchar != ' ' && p[0] == ' ' ) // and there's a space
				count++;

			lastchar = p[0];
			p++;
		}
		if( count )
		{
			char *start;
			lastchar = ' '; // auto continue spaces...
			//lprintf( "Looks like we have %d args", count );
			pp = (*pppArgs) = (char**)Allocate( sizeof( char * ) * ( count + 2 ) );
			p = newargs;
			count = 0;
			start = p;
			pp[0] = NULL; // to be filled in by a copy of the program name
			pp++;
			pp[0] = NULL;
			while( p[0] )
			{
				//lprintf( "check character %c %c", lastchar, p[0] );
				if( lastchar != ' ' && p[0] == ' ' ) // and there's a space
				{
					p[0] = 0;
					pp[0] = (char*)Allocate( strlen( start ) + 1 );
					pp[0] = StrDup( start );
					//lprintf( "Copied argument %d %s", pp - (*pppArgs), pp[0] );
					lastchar = ' ';
					pp++;
					start = p;
				}
				else if( lastchar == ' ' && p[0] != ' ' )
				{
					//lprintf( "first char of argument?" );
					lastchar = p[0];
					start = p;
				}
				else
				{
					lastchar = p[0];
				}
				p++;
			}
			if( start )
			{
				//lprintf( "Setting arg %d to %s", pp - (*pppArgs), start );
				pp[0] = StrDup( start );
				pp[1] = NULL;
			}
			else
			{
				//lprintf( "No arguments." );
				pp[0] = NULL;
			}

		}
		else
			(*pppArgs) = NULL;
	}
}

static uintptr_t CPROC SetTaskArguments( uintptr_t psv, arg_list args )
{
	PARAM( args, char *, taskargs );
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	//lprintf( "args to build %s", taskargs );
	BuildArgs( &task->args, taskargs );
	//lprintf( "%p result array..", task->args );
	//		if( task->args )
	//		{
	//			char **p = task->args;
	//         lprintf( "Have some args..." );
	//         while( p[0] )
	//				lprintf( "Arg: %s", (p++)[0] );
	//		}
	return psv;
}

//--------------------------------------------------------------------------

static uintptr_t CPROC CreateTaskWithName( uintptr_t psv, arg_list args )
{
	PARAM( args, char *, name );
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	if( task )
	{
		AddLink( &l.tasks, task );
	}

	task = (PMYTASK_INFO)Allocate( sizeof( MYTASK_INFO ) );
	MemSet( task, 0, sizeof( MYTASK_INFO ) );
	task->min_launch_time = 250; // 250 milliseconds to restart... configurable per task
	task->ready_timeout = 2000;
	task->launch_threshold_count = 1;
	task->name = StrDup( name );
	return (uintptr_t)task;
}

static uintptr_t CPROC SetSuspendStartup( uintptr_t psv, arg_list args )
{
	l.flags.bSuspend = TRUE;
	return psv;
}

static uintptr_t CPROC SetTaskSuspend( uintptr_t psv, arg_list args )
{
	PARAM( args, LOGICAL, bSuspend );
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	if( task )
	{
		task->flags.bSuspend = bSuspend;
	}
	return psv;
}

//--------------------------------------------------------------------------

static uintptr_t CPROC CreateRemoteTaskWithName( uintptr_t psv, arg_list args )
{
	PARAM( args, char *, name );
	PARAM( args, char *, address );
	PMYTASK_INFO task = (PMYTASK_INFO)Allocate( sizeof( MYTASK_INFO ) );
	MemSet( task, 0, sizeof( MYTASK_INFO ) );
	task->flags.bRemote = TRUE;
	// resolve this address here?  maybe it's not even valid?
	task->remote_address = StrDup( address );
	task->name = StrDup( name );
	return (uintptr_t)task;
}

//--------------------------------------------------------------------------

static PMYTASK_INFO FindTask( char *address, char *taskname )
{
	if( address )
	{
		lprintf( "Remote tasks are not yet supported..." );
	}
	else
	{
		INDEX idx;
		PMYTASK_INFO task;
		if( !taskname )
			return NULL;
		LIST_FORALL( l.tasks, idx, PMYTASK_INFO, task )
		{
			if( stricmp( task->name, taskname ) == 0 )
			{
#ifdef DEBUG_WAIT_LIST
				lprintf( "Task at idx %" _32fX "", idx );
#endif
				return task;
			}
		}
	}
	return NULL;
}

//--------------------------------------------------------------------------

static void ScheduleTask( PMYTASK_INFO task )
{
	uint32_t tick;
#ifdef DEBUG_WAIT_LIST
	lprintf( "schedule task." );
#endif
	if( !task )
		return;
	if( task->flags.bRestart )
	{
		INDEX idx;
		POINTER spawn;
		LIST_FORALL( task->spawns, idx, POINTER, spawn )
		{
			if( spawn )
			{
				lprintf( "already spawned..." );
				break;
			}
		}
		if( spawn )
		{
			lprintf( "Restartable task is already started." );
			return;
		}
	}
	if( task->flags.bSuspend )
	{
		lprintf( "Task suspended - not scheduling." );
		return;
	}
	if( task->flags.bScheduling )
	{
		lprintf( "Task %s is already being scheduled... circular dependancy?", task->name );
		return;
	}
	task->flags.bScheduling = 1;
	tick = GetTickCount();
	if( task->last_launch_time )
	{
		if( ( tick - task->last_launch_time ) < task->min_launch_time )
		{
			task->flags.bTooFast = 1;
			lprintf( "Choked restart of %s... ended %" _32fX "ms ago... speed violation of %" _32fX "\n"
					 , task->name
					 , tick - task->last_launch_time
					 , task->min_launch_time
					 );
			RelinkThing( l.choked, task );
			return;
		}
	}
	task->last_launch_time = tick;
	if( !l.flags.bSuspend )
	{
		PREQUIRED_TASK req_task;
		INDEX idx;
		LIST_FORALL( task->requires, idx, PREQUIRED_TASK, req_task )
		{
			if( !req_task->task->flags.bScheduled )
				ScheduleTask( req_task->task );
		}
		//lprintf( "Scheduling %s", task->name );
		RelinkThing( l.schedule, task );
		task->flags.bScheduled = 1;
	}
	task->flags.bScheduling = 0;
}

//--------------------------------------------------------------------------

static void ScheduleTasks( void )
{
	PMYTASK_INFO task;
	INDEX idx;
	if( !l.flags.bSuspend )
	{
		LIST_FORALL( l.tasks, idx, PMYTASK_INFO, task )
		{
			lprintf( "Scheduling... %s", task->name );
			ScheduleTask( task );
		}
	}
}

//--------------------------------------------------------------------------

static uintptr_t CPROC EndTaskDefs( uintptr_t psv )
{
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	AddLink( &l.tasks, task );
	return 0;
}

//--------------------------------------------------------------------------

static void WriteConfig( char *name )
{
	FILE *out = fopen( name, "wt" );
	if( out )
	{
		INDEX idx;
		PMYTASK_INFO info;
		fprintf( out, "%ssuspend startup", l.flags.bSuspend?"":"#" );
		fprintf( out, "\n# service is another keyword that should be implememtned \n\n\n" );
		fprintf( out, "#[Remote] task <some task name>   #create a new task referenced by name\n\n" );
		LIST_FORALL( l.tasks, idx, PMYTASK_INFO, info )
		{
			fprintf( out, "%stask %s%s%s\n"
					 , info->remote_address?"Remote ":""
					 , info->remote_address?info->remote_address:""
					 , info->remote_address?"@":""
					 ,info->name );
			if( info->args )
				fprintf( out, "args %s\n", GetArgsString( (PCTEXTSTR)info->args ) );
			else
				fprintf( out, "#args <arguments to program>\n" );
			fprintf( out, "program %s\n", info->program );
			if( info->path )
				fprintf( out, "path %s\n", info->path );
			else
				fprintf( out, "#path <working directory> # set where the task should start\n" );
			fprintf( out, "suspend %s\n", info->flags.bSuspend?"Yes":"No" );
			fprintf( out, "exclusive %s\n", info->flags.bExclusive?"Yes":"No" );
			fprintf( out, "restart %s\n", info->flags.bRestart?"Yes":"No" );
			if( info->requires )
			{
				INDEX idx;
				PREQUIRED_TASK req;
				LIST_FORALL( info->requires, idx, PREQUIRED_TASK, req )
				{
					fprintf( out, "%s %s%s%s\n"
							 , req->flags.bOptional?"suggest":"require"
							 , req->address?req->address:""
							 , req->address?"@":""
							 , req->name
							 );
				}
			}
			else
			{
				fprintf( out, "#require [system@]<some task name> # system is optional, but requires an @ if used\n" );
				fprintf( out, "#suggest [system@]<some task name> # task name is not optional, and should match a Task <name> entry\n" );
			}
			// if I had a list of services to load...
			//fprintf( out, "service %s\n", LoadExternalService );


			// if not assume ready, the task is known to be able to
			// use the construct library to indicate that it is ready.
			fprintf( out, "assume ready in %"_32f " milliseconds\n", info->ready_timeout );
			fprintf( out, "min launch time is %"_32f " milliseconds\n", info->min_launch_time );
			fprintf( out, "max launches in launch time is %"_32f "\n", info->launch_threshold_count );
			fprintf( out, "\n\n" );
		}
		fclose( out );
	}
}
//--------------------------------------------------------------------------

static void ReadConfig( void )
{
	//uintptr_t psvTask;
	PCONFIG_HANDLER pch;
	pch = CreateConfigurationHandler();
	AddConfigurationMethod( pch, "suspend startup", SetSuspendStartup );
	AddConfigurationMethod( pch, "task %m", CreateTaskWithName );
		 // this is probably useless... since the require line is where
		 // the address is actually specified...
	AddConfigurationMethod( pch, "Remote Task %m@%m", CreateRemoteTaskWithName );
	AddConfigurationMethod( pch, "program %m", SetTaskProgram );
	AddConfigurationMethod( pch, "suspend %b", SetTaskSuspend );
	AddConfigurationMethod( pch, "args %m", SetTaskArguments );
	AddConfigurationMethod( pch, "path %m", SetTaskPath );
	AddConfigurationMethod( pch, "require %m", AddTaskRequirement );
	AddConfigurationMethod( pch, "suggest %m", AddTaskSuggestion );
	AddConfigurationMethod( pch, "require %m@%m", AddRemoteTaskRequirement );
	AddConfigurationMethod( pch, "restart %b", SetTaskRestart );
	AddConfigurationMethod( pch, "service %m", LoadExternalService );
	AddConfigurationMethod( pch, "exclusive %b %m", SetTaskExclusive );
		 // if not assume ready, the task is known to be able to
		 // use the construct library to indicate that it is ready.
	AddConfigurationMethod( pch, "assume ready in %i milliseconds", SetTaskReadyTimeout );
	AddConfigurationMethod( pch, "min launch time is %i milliseconds", SetTaskMinLaunchTime );
	SetConfigurationEndProc( pch, EndTaskDefs );
	ProcessConfigurationFile( pch, l.configfile, 0 );
	DestroyConfigurationHandler( pch );

	WriteConfig( l.configfile );
	// void ValidateTasks()
	{
	// for all tasks, fixup requirement lists...
		INDEX idx;
		PMYTASK_INFO task;
		LIST_FORALL( l.tasks, idx, PMYTASK_INFO, task )
		{
			PREQUIRED_TASK required;
			INDEX idx;
			if( !task->name )
			{
				lprintf( "No task name!?" );
				Release( task );
				SetLink( &l.tasks, idx, NULL );
				continue;
			}
			else if( !( task->program ) )
			{
				lprintf( "No program specified for task %s", task->name );
				Release( task->name );
				Release( task );
				SetLink( &l.tasks, idx, NULL );
				continue;
			}
			//lprintf( "had a task... and it has requirements?" );
			if( !task->requires )
			{
				lprintf( "Task %s requires nothing.", task->name );
			}
			if( !task->args )
			{
				task->args = (char**)Allocate( sizeof( char *) * 2 );
				task->args[1] = NULL;
			}
			task->args[0] = StrDup( task->program );
			LIST_FORALL( task->requires, idx, PREQUIRED_TASK, required )
			{
				//lprintf( "yes..." );
				if( !( required->task = FindTask( required->address, required->name ) ) )
				{
					fprintf( stderr, "Failed to find dependancy %s for %s\n"
							 , required->name
							 , task->name );
					if( required->address ) Release( required->address );
					if( required->name ) Release( required->name );
					Release( required );
					SetLink( &task->requires, idx, NULL ); // delete this requirement
				}
				else
				{
#ifdef DEBUG_WAIT_LIST
					lprintf( "Found task %p(%"_32f ")...", required->task, FindLink( &l.tasks, required->task ) );
#endif
					AddLink( &required->task->required_by, task );
				}
			}
		}
	}
	// have validated tasks, now schedule them...
	ScheduleTasks();
}

//--------------------------------------------------------------------------

static void CPROC TaskStartOneShot( uintptr_t psv )
{
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	// if the task has not
	// already responded...
	if( !task->flags.bStarted )
	{
		task->flags.bStarted = 1;
		// all is well... continue starting tasks...
		WakeThread( l.pLoadingThread );
	}
}

//--------------------------------------------------------------------------

static void KillDependants( PMYTASK_INFO task )
{
	// this small bit of code needs to be recursive...
	// so that we kill all dependants of dependants of this task...
	PMYTASK_INFO dependant;
	INDEX idx;
	//lprintf( "A GOOD TASK IS DEAD!" );
	LIST_FORALL( task->required_by, idx, PMYTASK_INFO, dependant )
	{
		INDEX idx2;
		PTASK_INFO info;
		lprintf( "Terminating dependant program %s", dependant->name );
		//dependant->flags.bStopped = 1;
		LIST_FORALL( dependant->spawns, idx2, PTASK_INFO, info )
		{
			lprintf( "begin terminate" );
			TerminateProgram( info );
			lprintf( "done terminate" );
		}
	}
}


//--------------------------------------------------------------------------

static void CPROC TaskEnded( uintptr_t psv, PTASK_INFO task_ended )
{
	PMYTASK_INFO task = (PMYTASK_INFO)psv;
	lprintf( "Task %s has exited.", task->name );
	lprintf( "finding task info in task->spawns %p %p", &task->spawns, task_ended );
	if( task->flags.bWaiting )
	{
		lprintf( "Task %s exited while we were waiting for it to begin.\n", task->name );
		task->flags.bFailedSpawn = 1;
 	}
	if( task->flags.bWaitForStart || task->flags.bWaitForReady )
	{
		lprintf( "Task got a little further but still died before it began...\n" );
		lprintf( "It appears to be slightly summoner aware.\n" );
		task->flags.bFailedSpawn = 1;
	}
	lprintf( "last spawn is %p %d %d %d", l.current_task
			 , task->flags.bStopped
			 , task->flags.bRestart
			 , task->flags.bFailedSpawn
			 );

	lprintf( "this task is %p", task );
	task->flags.bWaiting = 0;
	task->flags.bStarted = 0;
	if( FindLink( &task->spawns, (POINTER)task_ended ) != INVALID_INDEX )
	{
		//lprintf( "Killing dep" );
		KillDependants( task );
		//lprintf( "Delete from spawn" );
		DeleteLink( &task->spawns, (POINTER)task_ended );
		if( !task->flags.bStopped
			&& task->flags.bRestart
			&& !task->flags.bFailedSpawn )
			ScheduleTask( task );
		//lprintf( "wake loader" );
		WakeThread( l.pLoadingThread );
		//DebugBreak();
	}
	else
	{
		lprintf( "Task wasn't started... why am I here?" );
		DebugBreak(); // this should basically NEVER happen.
		// if I didn't get a task_info, then this routine will NOT be
		// called.
	}
}

//--------------------------------------------------------------------------

static int CPROC WaitingForDependancies( PMYTASK_INFO task )
{
	INDEX idx;
	PREQUIRED_TASK required;
	LIST_FORALL( task->requires, idx, PREQUIRED_TASK, required )
	{
		// if it's opional, and the launch failed... not waiting
		// bfailedspawn is worse than btoofast
		if( required->flags.bOptional &&
			required->task->flags.bFailedSpawn )
			return 0;
		if( !required->task->flags.bStarted )
		{
#ifdef DEBUG_WAIT_LIST
			lprintf( "required task %s not started...", required->task->name );
#endif
			return 1;
		}
	}
	return 0;
}

//--------------------------------------------------------------------------

static void CPROC TaskOut(uintptr_t psvTask, PTASK_INFO task, CTEXTSTR buffer, size_t size ) {
	PMYTASK_INFO myTask = (PMYTASK_INFO)psvTask;
	if( buffer[size-1] == '\n' ) {
		if( size > 1 ) {
			lprintf( "%s:%*.*s", myTask->name, size-1, size-1, buffer );
		}
	} else
		lprintf( "%s:%*.*s", myTask->name, size, size, buffer );

}

//--------------------------------------------------------------------------

static uintptr_t taskStartWait( PTHREAD thread ) {
	PMYTASK_INFO task = (PMYTASK_INFO)GetThreadParam( thread );
	uint32_t tick = GetTickCount();
	//lprintf( "Waiting for task start in thread...%d", task->ready_timeout );
	while( task->flags.bWaiting
		  && ( ( tick + task->ready_timeout ) > GetTickCount() ) )
	{
		// should sleep-wait here... but really
		// it shouldn't take THAT long...
		//lprintf( "waiting..." );
		IdleFor( 250 );
		//Relinquish();
	}
	//lprintf( "Done waiting... result?" );
	if( task->flags.bWaiting )
	{
		lprintf( "Task %s appears to not be summoner aware.\n", task->name );
		//lprintf( "Forcing start...\n" );
		task->flags.bWaiting = 0;
		task->flags.bStarted = 1;
		RelinkThing( l.ready, task );
		WakeThread( l.pLoadingThread );
	}
	return 0;
}


static void LoadTasks( void )
{
	int started;
	PMYTASK_INFO task;
	l.pLoadingThread = MakeThread();
	do
	{
		PMYTASK_INFO nextTask;
		started = 0;
		if( l.flags.bSpawnActive )
		{
			lprintf( "spawning..." );
			return;
		}
		// don't load anything while suspended
		if( l.flags.bSuspend )
		{
			lprintf( "suspended." );
			return;
		}
#ifdef DEBUG_WAIT_LIST
		lprintf( "Searching for something to start..." );
#endif
		for( task = l.schedule; task; task = nextTask )
		{
			PTASK_INFO task_info;
			nextTask = NextThing( task );
			// next task in for loop.
#ifdef DEBUG_WAIT_LIST
			lprintf( "check %p(%s) %d", task, task?task->name:"", l.flags.bSpawnActive );
#endif
			if( l.flags.bSpawnActive )
				if( task->flags.bStarted )
				{
					lprintf( "Task started... and still in list?" );
					DebugBreak();
					UnlinkThing( task );
					continue;
				}
			if( task->flags.bWaiting )
			{
#ifdef DEBUG_WAIT_LIST
				lprintf( "Task waiting - try another peer dependancy" );
#endif
				continue;
			}
			if( WaitingForDependancies( task ) )
			{
#ifdef DEBUG_WAIT_LIST
				lprintf( "Task %s is waiting for depends.", task->name );
#endif
				continue;
			}
#ifdef DEBUG_WAIT_LIST
			lprintf( "Launching program... %s %s%s%s\n", task->name
					 , task->program
					 , task->path?" in ":"", task->path?task->path:"" );
#endif
			if( task->args )
			{
				char **p = task->args;
				while( p[0] )
					lprintf( "Arg: %s", (p++)[0] );
			}
			task->flags.bWaiting = 1;
			task->launch_count++;
			// this should only contain one thing,
			// however for ease of moving this node
										// from list to list, this should be regarded as a list.
			//DebugBreak();
			RelinkThing( l.current_task, task );
			l.flags.bSpawnActive = 1;
			started++;
			task->last_launch_time = GetTickCount();
			task_info = LaunchPeerProgramExx( task->program
			                                , task->path
			                                , (PCTEXTSTR)task->args
			                                , LPP_OPTION_DO_NOT_HIDE|LPP_OPTION_NEW_GROUP
			                                , TaskOut
			                                , TaskEnded, (uintptr_t)task DBG_SRC );
			if( task_info )
			{
				AddLink( &task->spawns, task_info );
				ThreadTo( taskStartWait, (uintptr_t)task );
			}
			else
			{
				lprintf( "failed spawn %p (%s)", task, task->name );
				// task ended callback may still get invoked?
				//lprintf( "in fact, END TASK may have already been invoked, but WILL definatly get invoked." );
				task->flags.bFailedSpawn = 1;
 			}
			l.flags.bSpawnActive = 0;
		}
	} while( started );
}

//--------------------------------------------------------------------------


//--------------------------------------------------------------------------

static void UnloadTask( PMYTASK_INFO task, LOGICAL bShutdown )
{
	INDEX idx2;
	PTASK_INFO info;
	KillDependants( task );
	lprintf( "Terminating program %s", task->name );
	task->flags.bStopped = bShutdown;
	LIST_FORALL( task->spawns, idx2, PTASK_INFO, info )
	{
		lprintf( "begin terminate" );
		TerminateProgram( info );
		lprintf( "end terminate" );
	}
}

static void UnloadTasks( void )
{
	INDEX idx;
	PMYTASK_INFO task;
	LIST_FORALL( l.tasks, idx, PMYTASK_INFO, task )
	{
		UnloadTask( task, TRUE );
	}
}

//--------------------------------------------------------------------------

#ifndef __cplusplus
ATEXIT( DoUnloadTasks )
{
	UnloadTasks();
}
#endif

//--------------------------------------------------------------------------

static void CPROC AvatarReadComplete( PCLIENT pc, POINTER buffer, size_t len )
{
	uint32_t toread;
	if( !buffer )
	{
		PNETSTATE pns;
		buffer = Allocate( 4096 );
		SetNetworkLong( pc, NL_BUFFER, (uintptr_t)buffer );

		SetNetworkLong( pc, NL_STATE, (uintptr_t)(pns=(PNETSTATE)Allocate( sizeof( NETSTATE ) )) );
		pns->state = NET_STATE_RESET;
		toread = 4;
	}
	else
	{
		PNETSTATE pns = (PNETSTATE)GetNetworkLong( pc, NL_STATE );
		uint32_t* msg = (uint32_t*)buffer;
		lprintf( "Received... ");
		LogBinary( (uint8_t*)buffer, len );
		switch( pns->state )
		{
		case NET_STATE_RESET:
			pns->state = NET_STATE_COMMAND;
			// continue through to correct state after reset.
		//break;
		case NET_STATE_COMMAND:
			switch( pns->cmd = msg[0] )
			{
			case SUMMONER_TASK_START:
			case SUMMONER_TASK_STOP:
			case SUMMONER_TASK_SUSPEND:
			case LIST_TASK_DEPENDS:
				pns->state = NET_STATE_GET_TASKID;
				toread = 4;
				break;
			case LIST_TASKS:
				{
					uint32_t op = TASK_NAME;
					PMYTASK_INFO task;
					INDEX idx;
					LIST_FORALL( l.tasks, idx, PMYTASK_INFO, task )
					//for( task = l.tasks; task; task = NextThing( task ) )
					{
						uint32_t op = TASK_NAME;
						char *state;
						char line[80];
						if( task->flags.bStarted )
							state ="ready";
						else if( task->flags.bWaiting )
							state ="wait1";
						else if( task->flags.bWaitForStart )
							state ="Wait2";
						else if( task->flags.bWaitForReady )
							state ="Wait3";
						else
							state ="dead";

						SendTCP( pc, &op, sizeof( op ) );
						SendTCP( pc, &idx, sizeof( idx ) );
						len = snprintf( line, sizeof( line ), "%s\t%s"
										  , task->name, state );
						lprintf( "Sending back task index %"_32f " = %s(%s)", idx, task->name, state );
						SendTCP( pc, &len, sizeof( len ) );
						SendTCP( pc, line, len );
					}
					op = TASK_LIST_DONE;
					SendTCP( pc, &op, sizeof( op ) );
					toread = 4;
					pns->state = NET_STATE_COMMAND;
				}
				break;
			}
			break;
		case NET_STATE_GET_TASKID:
			{
				switch( pns->cmd )
				{
				case LIST_TASK_DEPENDS:
					{
						uint32_t op = TASK_DEPEND;
						INDEX cnt = 0;
						INDEX idx = ((INDEX*)buffer)[0];
						PMYTASK_INFO task = (PMYTASK_INFO)GetLink( &l.tasks, idx );
						PREQUIRED_TASK required;

						lprintf( "Sending dependancies for %"_32f "", idx );
						LIST_FORALL( task->requires, idx, PREQUIRED_TASK, required )
							cnt++;
						// send task_id back
						SendTCP( pc, &op, sizeof( op ) );
						SendTCP( pc, buffer, 4 );
						// send count of depends
						SendTCP( pc, &cnt, sizeof( cnt ) );

						LIST_FORALL( task->requires, idx, PREQUIRED_TASK, required )
						{
							INDEX task_idx = FindLink( &l.tasks, required->task );
							lprintf( "Sending task %p(%"_32f ") for %"_32f "", required->task, task_idx, ((INDEX*)buffer)[0] );
							SendTCP( pc, &task_idx, sizeof( task_idx ) );
						}
						pns->state = NET_STATE_COMMAND;
						toread = 4;
					}
					break;
				case SUMMONER_TASK_STOP:
					{
						INDEX idx = ((INDEX*)buffer)[0];
						PMYTASK_INFO task = (PMYTASK_INFO)GetLink( &l.tasks, idx );
						if( task->flags.bStarted )
						{
							UnloadTask( task, TRUE );
						}
						pns->state = NET_STATE_COMMAND;
						toread = 4;
					}
					break;
				case SUMMONER_TASK_START:
					{
						INDEX idx = ((INDEX*)buffer)[0];
						PMYTASK_INFO task = (PMYTASK_INFO)GetLink( &l.tasks, idx );
						// put this task in the started list.
						if( !task->flags.bStarted )
						{
							task->flags.bStopped = 0;
							LinkThing( l.schedule, task );
							WakeThread( l.pLoadingThread );
						}
						pns->state = NET_STATE_COMMAND;
						toread = 4;
					}

					break;
				case SUMMONER_TASK_SUSPEND:
					{
						//INDEX idx = ((INDEX*)buffer)[0];
						//PMYTASK_INFO task = (PMYTASK_INFO)GetLink( &l.tasks, idx );
						pns->state = NET_STATE_COMMAND;
						toread = 4;
					}

					break;
				default:
					// getting a task index for an invalid protocol message;
					break;
				}
				break;
			}
		}
	}
	lprintf( "Do read.. %"_32f "", toread );
	ReadTCPMsg( pc, buffer, toread );
}

//--------------------------------------------------------------------------

static void CPROC AvatarConnect( PCLIENT pcServer, PCLIENT pcNew )
{
	lprintf( "connecting, and responding with status..." );
	SetReadCallback( pcNew, AvatarReadComplete );
	//AvatarReadComplete( pcNew, NULL, 0 );
	{
		struct {
			uint32_t op;
			uint32_t system_id;
			uint32_t status;
		} msg;
		msg.op = SYSTEM_STATUS;
		msg.system_id = 0;
		msg.status = l.flags.bSuspend;
		SendTCP( pcNew, &msg, sizeof( msg ) );
	}
}

//--------------------------------------------------------------------------
static void CPROC SummonerReadComplete( PCLIENT pc, POINTER buffer, size_t len )
{
	uint32_t toread;
	if( !buffer )
	{
		PNETSTATE pns;
		buffer = Allocate( 4096 );
		SetNetworkLong( pc, NL_BUFFER, (uintptr_t)buffer );

		SetNetworkLong( pc, NL_STATE, (uintptr_t)(pns=(PNETSTATE)Allocate( sizeof( NETSTATE ) )) );
		pns->state = NET_STATE_RESET;
		toread = 4;
	}
	else
	{
		struct summonerMessage {
			int op;
			union {
				char requirement[1];
			} data;	
		} *msg = (struct summonerMessage*)buffer;
		
	}
}

//--------------------------------------------------------------------------


void CPROC SummonerConnect( PCLIENT pcServer, PCLIENT pcNew )
{
	lprintf( "connection from peer summoner..." );
	SetReadCallback( pcNew, SummonerReadComplete );
}

//--------------------------------------------------------------------------


SaneWinMain( argc, argv )
{
	// process a configuration file for tasks to load
	// also register a service for which I can be notified of task
	// health, before performing a kill and resurrect....
	if( argc > 1 )
	{
		l.configfile = argv[1];
	}
	else
	{
		l.configfile = "summoner.config";
	}
	if( !RegisterServiceHandler( SUMMONER_NAME, SummonerEvents ) )
	{
		LoadFunction( PREFIX "sack.msgsvr.service.plugin", NULL );
		// this should be deterministic enough ...
		// loading the task there's no reliablibity, and no way to event notify
		// completion... if there was already a message service, then this would
		// have registered, and not required loading the service.
		// the service becomes available just by loading the library, no init function
		// is required.
		if( !RegisterServiceHandler( SUMMONER_NAME, SummonerEvents ) )
		{
			fprintf( stderr, "Could not register myself as the master service.  Perhaps I am already loaded?" );
			return -1;
		}
	}
#ifdef WIN32
	RegisterIcon( NULL );
#endif

	l.peer_port = 3014;
	l.avatar_port = 3015;
	//ServiceRespond( l.peer_port );
	//DiscoverService( 0, l.port, NULL, 0 );
	NetworkWait( NULL, 16, 2);
	if( !OpenTCPListenerEx( l.avatar_port, AvatarConnect ) )
	{
		lprintf( "Failed to open avatar tcp port (%d)", l.avatar_port );
		return 0;
	}
	if( !OpenTCPListenerEx( l.peer_port, SummonerConnect ) )
	{
		lprintf( "Failed to open summoner tcp port (%d)", l.peer_port );
		return 0;
	}
	ReadConfig();
					  // and then we were done...
	//l.flags.bSuspend = TRUE;
	while( 1 )
	{
		// attempt to load some tasks...
		LoadTasks();
		WakeableSleep( SLEEP_FOREVER );
		lprintf( "Woke up?!" );
	}
	return 0;
}
EndSaneWinMain()

