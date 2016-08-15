#ifdef WIN32
#include <systray.h>
#endif
#include <sharemem.h>
#include <configscript.h>
#include <netservice.h>
#include <sack_system.h>
#include <timers.h>
#include <network.h>

typedef struct task_info_tag {
	DeclareLink( struct task_info_tag );
	char *fluffyname;
	struct {
		uint32_t bRespawn : 1;
		uint32_t bKilled : 1; // don't restart until allowed...
	} flags;
	char *program;
	char *path;
	char **args;
   PLIST requires;
   PTASK_INFO info;
} MY_TASK_INFO, *PMY_TASK_INFO;

typedef struct global_tag {
	PMY_TASK_INFO tasks;
   uint32_t port;
} LOCAL;

static LOCAL g;

//-------------------------------------------------------------------------

void CPROC TaskEnded( uintptr_t psv, PTASK_INFO info )
{
	PMY_TASK_INFO task = (PMY_TASK_INFO)psv;
	if( task )
	{
		if( task->flags.bRespawn && !task->flags.bKilled )
         LaunchProgramEx( task->program, task->path, task->args, TaskEnded, (uintptr_t)task );
	}
}

//-------------------------------------------------------------------------

void KillTasks( void )
{
	PMY_TASK_INFO task = g.tasks;
	while( task )
	{
      task->flags.bKilled = 1;
		TerminateProgram( task->info );
      task = NextLink( task );
	}
}

//-------------------------------------------------------------------------

void StartTasks( void )
{
	PMY_TASK_INFO task;
	for( task = g.tasks; task; task = NextLink( task ) )
	{
		if( !task->info )
			task->info = LaunchProgramEx( task->program, task->path, task->args, TaskEnded, (uintptr_t)task );
		if( !task->info )
		{
			lprintf( WIDE("Failed to load task %s (%s in %s)"), task->fluffyname, task->program, task->path );
			UnlinkThing( task );
         if( task->fluffyname )
				Release( task->fluffyname );
         if( task->program )
				Release( task->program );
         if( task->path )
				Release( task->path );
			if( task->args )
			{
				{
					char **p;
					for( p = task->args; p && *p; p++ )
						Release( *p );
				}
				Release( task->args );
			}
         Release( task );
		}
	}
}

//-------------------------------------------------------------------------

uintptr_t CPROC NewTask( uintptr_t psv, va_list args )
{
	PARAM( args, char *, name );
	PMY_TASK_INFO task = Allocate( sizeof( MY_TASK_INFO ) );
   MemSet( task, 0, sizeof( MY_TASK_INFO ) );
	if( psv )
	{
      LinkThing( g.tasks, (PMY_TASK_INFO) psv );
	}
	task->fluffyname = StrDup( name );
   return (uintptr_t)task;
}

//-------------------------------------------------------------------------

uintptr_t CPROC SetProgramName( uintptr_t psv, va_list args )
{
	PARAM( args, char *, name );
	PMY_TASK_INFO task = (PMY_TASK_INFO)psv;
	if( task )
	{
      task->program = StrDup( name );
	}
   return psv;
}

//-------------------------------------------------------------------------

uintptr_t CPROC SetProgramPath( uintptr_t psv, va_list args )
{
	PARAM( args, char *, path );
	PMY_TASK_INFO task = (PMY_TASK_INFO)psv;
	if( task )
	{
      task->path = StrDup( path );
	}
   return psv;
}

//---------------------------------------------------------------------------

//-------------------------------------------------------------------------

void SetTaskArguments( PMY_TASK_INFO task, char *args )
{
	char *p, **pp;
	pp = task->args;
	while( pp && pp[0] )
	{
		Release( pp[0] );
		pp++;
	}
	Release( task->args );

	{
		int count = 0;
		int lastchar;
		lastchar = ' '; // auto continue spaces...
		lprintf( WIDE("Got args: %s"), args );
		p = args;
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
			pp = task->args = Allocate( sizeof( char * ) * ( count + 1 ) );
			p = args;
			count = 0;
			start = p;
			pp[0] = NULL;
			while( p[0] )
			{
				lprintf( WIDE("check character %c %c"), lastchar, p[0] );
				if( lastchar != ' ' && p[0] == ' ' ) // and there's a space
				{
					p[0] = 0;
					pp[0] = Allocate( strlen( start ) + 1 );
					pp[0] = StrDup( start );
					lprintf( WIDE("Copied argument %d %s"), pp - task->args, pp[0] );
					lastchar = ' ';
					pp++;
					start = p;
				}
				else if( lastchar == ' ' && p[0] != ' ' )
				{
					lprintf( WIDE("first char of argument?") );
					lastchar = p[0];
					start = p;
				}
				else
				{
					lastchar = p[0];
				}
				p++;
			}
			lprintf( WIDE("Setting arg %d to %s"), pp - task->args, start );
			if( start )
			{
				pp[0] = StrDup( start );
				pp[1] = NULL;
			}
			else pp[0] = NULL;

		}
		else
			task->args = NULL;
	}
}

//-------------------------------------------------------------------------

uintptr_t CPROC SetProgramArgs( uintptr_t psv, va_list args )
{
	PARAM( args, char *, progargs );
	PMY_TASK_INFO task = (PMY_TASK_INFO)psv;
	if( task )
	{
      SetTaskArguments( task, progargs );
	}
   return psv;
}

//-------------------------------------------------------------------------

uintptr_t CPROC SetProgramRespawn( uintptr_t psv, va_list args )
{
	PARAM( args, LOGICAL, respawn);
	PMY_TASK_INFO task = (PMY_TASK_INFO)psv;
	if( task )
	{
		task->flags.bRespawn = respawn;
	}
   return psv;
}

uintptr_t CPROC SetProgramRequirements( uintptr_t psv, va_list args )
{
   PARAM( args, char *, require );
	PMY_TASK_INFO task = (PMY_TASK_INFO)psv;
	if( task )
	{
      AddLink( &task->requires, StrDup( require ) );
	}
   return psv;
}

//-------------------------------------------------------------------------

uintptr_t CPROC TasksLoaded( uintptr_t psv )
{
	if( psv )
	{
      LinkThing( g.tasks, (PMY_TASK_INFO) psv );
	}
   return 0;
}

//-------------------------------------------------------------------------

void LoadTasks( void )
{
	PCONFIG_HANDLER pch = CreateConfigurationHandler();
	AddConfigurationMethod( pch, WIDE("[%m]"), NewTask );
	AddConfigurationMethod( pch, WIDE("program=%m"), SetProgramName );
	AddConfigurationMethod( pch, WIDE("path=%m"), SetProgramPath );
	AddConfigurationMethod( pch, WIDE("args=%m"), SetProgramArgs );
	AddConfigurationMethod( pch, WIDE("require=%m"), SetProgramRequirements );
	AddConfigurationMethod( pch, WIDE("restart=%b"), SetProgramRespawn );
	SetConfigurationEndProc( pch, TasksLoaded );
	ProcessConfigurationFile( pch, WIDE("startup.ini"), 0 );
   DestroyConfigurationHandler( pch );
}

//-------------------------------------------------------------------------

void DoTaskUpdate( void )
{
   // uhmm not sure what to do here...
}

//-------------------------------------------------------------------------

#define NL_BUFFER 0
#define NL_READ_SO_FAR 0

void CPROC ControllerReadComplete( PCLIENT pc, POINTER readbuf, int size )
{
	uint32_t ofs = GetNetworkLong( pc, NL_READ_SO_FAR );
   uint8_t* buffer = (uint8_t*)readbuf;
	if( !buffer )
	{
      buffer = (POINTER)GetNetworkLong( pc, NL_BUFFER );
	}
	else
	{
		if( size + ofs < 8 )
		{
			// zero fill the buffer... then we can compare on partial
			// words... so all that needs to be sent is UPDATE .... and
         MemSet( buffer + ofs + size, 0, 8 - ( ofs + size ) );
		}
      ofs += size;
		if( ((uint64_t*)buffer)[0] == *(uint64_t*)"UPDATE\0\0" )
		{
			DoTaskUpdate();
			ofs = 0;
		}
		else if( ((uint64_t*)buffer)[0] == *(uint64_t*)"RESTART\0" )
		{
			KillTasks();
			StartTasks();
			ofs = 0;
		}
		else if( ((uint64_t*)buffer)[0] == *(uint64_t*)"START\0\0\0" )
		{
			StartTasks();
			ofs = 0;
		}
		else if( ((uint64_t*)buffer)[0] == *(uint64_t*)"STOP\0\0\0\0" )
		{
			KillTasks();
			ofs = 0;
		}
      SetNetworkLong( pc, NL_READ_SO_FAR, ofs );
	}
   ReadTCP( pc, buffer + ofs, 4096 - ofs );
}

//-------------------------------------------------------------------------

void CPROC ControllerDisconnect( PCLIENT pc )
{
	POINTER buffer = (POINTER)GetNetworkLong( pc, NL_BUFFER );
   Release( buffer );
}

//-------------------------------------------------------------------------

void CPROC ControllerConnected( PCLIENT pcNew, PCLIENT pcServer )
{
	SetNetworkReadComplete( pcNew, ControllerReadComplete );
   SetNetworkLong( pcNew, NL_BUFFER, (uintptr_t)Allocate( 4096 ) );
}

void StartSpawnerService( void )
{
	PCLIENT pc;
   NetworkWait( NULL, 32, 10 );
	pc = OpenTCPListenerEx( g.port
								 , ControllerConnected );
   ServiceRespond( g.port );
}

//-------------------------------------------------------------------------

void Init( void )
{
	g.port = 3006;
}

//-------------------------------------------------------------------------

ATEXIT( KillAllTasks )
{
   KillTasks();
}

//-------------------------------------------------------------------------

int main( void )
{
#ifdef WIN32
   RegisterIcon( NULL );
#endif
   Init();
	LoadTasks();
	StartTasks();
   StartSpawnerService();
   if( g.tasks )
		Sleep( SLEEP_FOREVER );
	else
      lprintf( WIDE("Sorry, no tasks defined, nothing loaded, nothing done.") );
   return 0;
}
