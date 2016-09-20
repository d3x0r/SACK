#include <stdhdrs.h>
#include <stdio.h>
#include <network.h>
#include <logging.h>
#include <sharemem.h>

PCLIENT pcControl;

#define NL_LASTMSG 0

void CPROC CloseCallback(PCLIENT pc)
{
	lprintf( WIDE("\n*** Connection to relay is terminated ***\n") );
	pcControl = NULL;
}

void CPROC ReadComplete( PCLIENT pc, POINTER buffer, size_t size )
{
    static uint64_t LastMessage; // this is safe - only ONE connection EVER
    uint64_t test;
    int ToRead = 8;
    if( !buffer )
    {
        buffer = Allocate( 1024 );
    }
    else
    {
        ((TEXTCHAR*)buffer)[size] = 0;
        if( !LastMessage )
        {
            Log1( WIDE("Message is: %8.8s"), buffer );
            if( *(uint64_t*)buffer == *(uint64_t*)WIDE("USERLIST") )
            {
                ToRead = 2;
                LastMessage = *(uint64_t*)buffer;
            }
            else if( *(uint64_t*)buffer == *(uint64_t*)WIDE("USERDEAD") )
            {
                Log( WIDE("User has been terminated!\n") );
                RemoveClient( pc );
                return;
            }
            else if( *(uint64_t*)buffer == *(uint64_t*)WIDE("ALL DONE") )
            {
                RemoveClient( pc );
                return;
            }
            else if( *(uint64_t*)buffer == *(uint64_t*)WIDE("MESSAGE!") )
            {
                ToRead = 1;
                LastMessage = *(uint64_t*)buffer;
            }
            else if( *(uint64_t*)buffer == *(uint64_t*)WIDE("MASTERIS") )
            {
                LastMessage = *(uint64_t*)buffer;
            }
            else
            {
                lprintf( WIDE("Unknown responce from relay: %8.8s"), buffer );
            }
        }
        else
        {
            Log1( WIDE("Continuing message: %8.8s"), &LastMessage );
            if( LastMessage == *(uint64_t*)WIDE("MESSAGE!") )
            {
                Log( WIDE("(1)") );
                ToRead = *(uint8_t*)buffer;
                LastMessage++;
            }
            else if( (test = ((*(uint64_t*)WIDE("MESSAGE!"))+1)), (LastMessage == test) )
            {
                Log( WIDE("(2)") );
                lprintf( WIDE("Relay Message:%s"), buffer );
                LastMessage = 0;
            }
            else if( LastMessage == *(uint64_t*)WIDE("MASTERIS") )
            {
                lprintf( WIDE("Game Master is: %s"), buffer );
                LastMessage = 0;
                RemoveClient( pc );
            }
            else if( LastMessage == (*(uint64_t*)WIDE("USERLIST")) )
            {
                Log( WIDE("(3)") );
                ToRead = *(uint16_t*)buffer;
                Log1( WIDE("User list with size of %d"), ToRead );
                LastMessage++;
            }
            else if(  (test = ((*(uint64_t*)WIDE("USERLIST"))+1) ), (LastMessage == test) )
            {
                TEXTCHAR *userlist = (TEXTCHAR*)buffer;
                userlist[size] = 0;
                lprintf( WIDE("User List:\n%s"), userlist );
                LastMessage = 0;
                RemoveClient( pc );
            }
            else
            {
                Log1( WIDE("Unknown Continuation state for %8.8s"), &LastMessage );
            }
        }
    }
    ReadTCPMsg( pc, buffer, ToRead );
}

void usage( void )
{
   lprintf( WIDE("Commands are:\n")
      		WIDE("  update - send files specified in common to all connected\n")
      		WIDE("  who    - list who is currently connected\n")
      		WIDE("  kill   - terminate relay program\n")
      		WIDE("  time   - Get current time (unimplemented result)\n")
      		WIDE("  logoff - specify user to terminate\n")
          WIDE("  scan   - tell remote to scan his directories\n")
          WIDE("  status - get game master's status\n")
          WIDE("  reboot - tell remote to reboot >:)\n")
          WIDE("  winners - force a scan of the winners...\n")
      	);
}

SaneWinMain( argc, argv )
{
   SetSystemLog( SYSLOG_FILE, stdout );
   if( argc < 2 )
   {
   		usage();
		return 0;
   }
   NetworkWait( NULL, 6, 4 );
   pcControl = OpenTCPClientEx( WIDE("127.0.0.1"), 3001, ReadComplete, CloseCallback, NULL );
   if( !pcControl )
   		lprintf( WIDE("Relay was not running?\n") );
   else
   {
      {
         if( strcmp( argv[1], WIDE("update") ) == 0 )
         {
         	char msg[256];
         	int len;
         	len = sprintf( msg, "UPDATE  %s", argv[2]?argv[2]:WIDE("") );
	         SendTCP( pcControl, msg, len );
         }
         else if( strcmp( argv[1], WIDE("who") ) == 0 )
         {
         	lprintf( WIDE("Requesting active user list from relay.\n") );
         	SendTCP( pcControl, WIDE("LISTUSER"), 8 );
         }
         else if( strcmp( argv[1], WIDE("time") ) == 0 )
         {
         	SendTCP( pcControl, WIDE("GET TIME"), 8 );
         }
         else if( strcmp( argv[1], WIDE("kill") ) == 0 )
         {
	         SendTCP( pcControl, WIDE("DIE NOW!"), 8 );
         }
         else if( strcmp( argv[1], WIDE("scan") ) == 0 )
         {
         	char msg[256];
         	int len;
         	len = sprintf( msg, "DO SCAN!%s", argv[2]?argv[2]:WIDE("") );
	         SendTCP( pcControl, msg, len );
         }
         else if( strcmp( argv[1], WIDE("reboot") ) == 0 )
         {
         	char msg[256];
         	int len;
         	len = sprintf( msg, "REBOOT!!%s", argv[2] );
	         SendTCP( pcControl, msg, len );
         }
         else if( strcmp( argv[1], WIDE("status") ) == 0 )
         {
	         SendTCP( pcControl, WIDE("MASTER??"), 8 );
         }
         else if( strcmp( argv[1], WIDE("winners") ) == 0 )
         {
	         SendTCP( pcControl, WIDE("WINNERS!"), 8 );
         }
         else if( strcmp( argv[1], WIDE("logoff") ) == 0 )
         {
         	char msg[256];
         	int len;
         	len = sprintf( msg, "KILLUSER%s", argv[2] );
	         SendTCP( pcControl, msg, len );
         }
         else if( strcmp( argv[1], WIDE("memory") ) == 0 )
         {
             SendTCP( pcControl, "MEM DUMP", 8 );
         }
         else
         {
         	usage();
            return 0;
         }
      }
	   //lprintf( WIDE("Relay found, command issued - waiting for close.\n") );
   }
   // no wait - wait doesn't work right on unix :(
//#ifdef _WIN32
   while( pcControl )
      Sleep(10);
//#endif
}
EndSaneWinMain()

