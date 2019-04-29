#include <stdhdrs.h>
#include <stdio.h>
#include <network.h>
#include <logging.h>
#include <sharemem.h>

PCLIENT pcControl;

#define NL_LASTMSG 0

void CPROC CloseCallback(PCLIENT pc)
{
	lprintf( "\n*** Connection to relay is terminated ***\n" );
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
            Log1( "Message is: %8.8s", buffer );
            if( *(uint64_t*)buffer == *(uint64_t*)"USERLIST" )
            {
                ToRead = 2;
                LastMessage = *(uint64_t*)buffer;
            }
            else if( *(uint64_t*)buffer == *(uint64_t*)"USERDEAD" )
            {
                Log( "User has been terminated!\n" );
                RemoveClient( pc );
                return;
            }
            else if( *(uint64_t*)buffer == *(uint64_t*)"ALL DONE" )
            {
                RemoveClient( pc );
                return;
            }
            else if( *(uint64_t*)buffer == *(uint64_t*)"MESSAGE!" )
            {
                ToRead = 1;
                LastMessage = *(uint64_t*)buffer;
            }
            else if( *(uint64_t*)buffer == *(uint64_t*)"MASTERIS" )
            {
                LastMessage = *(uint64_t*)buffer;
            }
            else
            {
                lprintf( "Unknown responce from relay: %8.8s", buffer );
            }
        }
        else
        {
            Log1( "Continuing message: %8.8s", &LastMessage );
            if( LastMessage == *(uint64_t*)"MESSAGE!" )
            {
                Log( "(1)" );
                ToRead = *(uint8_t*)buffer;
                LastMessage++;
            }
            else if( (test = ((*(uint64_t*)"MESSAGE!")+1)), (LastMessage == test) )
            {
                Log( "(2)" );
                lprintf( "Relay Message:%s", buffer );
                LastMessage = 0;
            }
            else if( LastMessage == *(uint64_t*)"MASTERIS" )
            {
                lprintf( "Game Master is: %s", buffer );
                LastMessage = 0;
                RemoveClient( pc );
            }
            else if( LastMessage == (*(uint64_t*)"USERLIST") )
            {
                Log( "(3)" );
                ToRead = *(uint16_t*)buffer;
                Log1( "User list with size of %d", ToRead );
                LastMessage++;
            }
            else if(  (test = ((*(uint64_t*)"USERLIST")+1) ), (LastMessage == test) )
            {
                TEXTCHAR *userlist = (TEXTCHAR*)buffer;
                userlist[size] = 0;
                lprintf( "User List:\n%s", userlist );
                LastMessage = 0;
                RemoveClient( pc );
            }
            else
            {
                Log1( "Unknown Continuation state for %8.8s", &LastMessage );
            }
        }
    }
    ReadTCPMsg( pc, buffer, ToRead );
}

void usage( void )
{
   lprintf( "Commands are:\n"
      		"  update - send files specified in common to all connected\n"
      		"  who    - list who is currently connected\n"
      		"  kill   - terminate relay program\n"
      		"  time   - Get current time (unimplemented result)\n"
      		"  logoff - specify user to terminate\n"
          "  scan   - tell remote to scan his directories\n"
          "  status - get game master's status\n"
          "  reboot - tell remote to reboot >:)\n"
          "  winners - force a scan of the winners...\n"
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
   pcControl = OpenTCPClientEx( "127.0.0.1", 3001, ReadComplete, CloseCallback, NULL );
   if( !pcControl )
   		lprintf( "Relay was not running?\n" );
   else
   {
      {
         if( strcmp( argv[1], "update" ) == 0 )
         {
         	char msg[256];
         	int len;
         	len = sprintf( msg, "UPDATE  %s", argv[2]?argv[2]:"" );
	         SendTCP( pcControl, msg, len );
         }
         else if( strcmp( argv[1], "who" ) == 0 )
         {
         	lprintf( "Requesting active user list from relay.\n" );
         	SendTCP( pcControl, "LISTUSER", 8 );
         }
         else if( strcmp( argv[1], "time" ) == 0 )
         {
         	SendTCP( pcControl, "GET TIME", 8 );
         }
         else if( strcmp( argv[1], "kill" ) == 0 )
         {
	         SendTCP( pcControl, "DIE NOW!", 8 );
         }
         else if( strcmp( argv[1], "scan" ) == 0 )
         {
         	char msg[256];
         	int len;
         	len = sprintf( msg, "DO SCAN!%s", argv[2]?argv[2]:"" );
	         SendTCP( pcControl, msg, len );
         }
         else if( strcmp( argv[1], "reboot" ) == 0 )
         {
         	char msg[256];
         	int len;
         	len = sprintf( msg, "REBOOT!!%s", argv[2] );
	         SendTCP( pcControl, msg, len );
         }
         else if( strcmp( argv[1], "status" ) == 0 )
         {
	         SendTCP( pcControl, "MASTER??", 8 );
         }
         else if( strcmp( argv[1], "winners" ) == 0 )
         {
	         SendTCP( pcControl, "WINNERS!", 8 );
         }
         else if( strcmp( argv[1], "logoff" ) == 0 )
         {
         	char msg[256];
         	int len;
         	len = sprintf( msg, "KILLUSER%s", argv[2] );
	         SendTCP( pcControl, msg, len );
         }
         else if( strcmp( argv[1], "memory" ) == 0 )
         {
             SendTCP( pcControl, "MEM DUMP", 8 );
         }
         else
         {
         	usage();
            return 0;
         }
      }
	   //lprintf( "Relay found, command issued - waiting for close.\n" );
   }
   // no wait - wait doesn't work right on unix :(
//#ifdef _WIN32
   while( pcControl )
      Sleep(10);
//#endif
}
EndSaneWinMain()

