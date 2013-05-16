#include <stdhdrs.h>

#include <system.h>

typedef void (CPROC *PLINK_DataReceived)(PTRSZVAL psv, POINTER buffer, int length);
typedef void (CPROC *PLINK_ConnectionClosed)( PTRSZVAL psv );


typedef struct plink_tracker
{
	PTASK_INFO task;
	PLINK_DataReceived DataReceived;
	PLINK_ConnectionClosed ConnectionClosed;
   PTRSZVAL psv;

} *PSSH_PLINK_TRACKER, SSH_PLINK_TRACKER;

void CPROC PlinkEnd( PTRSZVAL psv, PTASK_INFO task )
{
	PSSH_PLINK_TRACKER tracker = (PSSH_PLINK_TRACKER)psv;
	tracker->ConnectionClosed( tracker->psv );
}

void CPROC PlinkReceive( PTRSZVAL psv, PTASK_INFO task, CTEXTSTR buffer, _32 size )
{
	PSSH_PLINK_TRACKER tracker = (PSSH_PLINK_TRACKER)psv;
	tracker->DataReceived( tracker->psv, buffer, size );
}

PLINK_TRACKER PlinkConnectSSH( CTEXTSTR address
				  , PLINK_DataReceived DataReceived
				  , PLINK_ConnectionClosed ConnectionClosed
				  , PTRSZVAL psv )
{
	PSSH_PLINK_TRACKER tracker = New( SSH_PLINK_TRACKER );
	PCTEXTSTR args;
	int nArgs;
	ParseIntoArgs( address, &nArgs, &args );
	tracker->psv = psv;
	tracker->DataRecieved = DataRecieved;
   tracker->ConnectionClosed = ConnectionClosed;
	tracker->task = LaunchPeerProgram( "plink", NULL, args, PlinkRecieve, PlinkEnd, (PTRSZVAL)tracker );
   return tracker;
}

int PlinkSendSSH( PSSH_PLINK_TRACKER link, CTEXTSTR format, ... )
{
	va_list args = va_start( format );
	return vpprintf( link->task, format, args );
}


