#include <stdhdrs.h>
#include <stdio.h>
#include <network.h>
#include <logging.h>
#include <sharemem.h>

PCLIENT pcControl;

#define NL_LASTMSG 0

void CPROC CloseCallback(PCLIENT pc)
{
	printf( "\n*** Connection to relay is terminated ***\n" );
	pcControl = NULL;
}

void CPROC ReadComplete( PCLIENT pc, POINTER buffer, int size )
{
    static _64 LastMessage; // this is safe - only ONE connection EVER
    _64 test;
    int ToRead = 8;
    if( !buffer )
    {
        buffer = Allocate( 1024 );
    }
    else
    {
        ((char*)buffer)[size] = 0;
        if( !LastMessage )
        {
            Log1( "Message is: %8.8s", buffer );
            if( *(_64*)buffer == *(_64*)"USERLIST" )
            {
                ToRead = 2;
                LastMessage = *(_64*)buffer;
            }
            else if( *(_64*)buffer == *(_64*)"USERDEAD" )
            {
                Log( "User has been terminated!\n" );
                RemoveClient( pc );
                return;
            }
            else if( *(_64*)buffer == *(_64*)"ALL DONE" )
            {
                RemoveClient( pc );
                return;
            }
            else if( *(_64*)buffer == *(_64*)"MESSAGE!" )
            {
                ToRead = 1;
                LastMessage = *(_64*)buffer;
            }
            else if( *(_64*)buffer == *(_64*)"MASTERIS" )
            {
                LastMessage = *(_64*)buffer;
            }
            else
            {
                printf( "Unknown responce from relay: %8.8s", buffer );
            }
        }
        else
        {
            Log1( "Continuing message: %8.8s", &LastMessage );
            if( LastMessage == *(_64*)"MESSAGE!" )
            {
                Log( "(1)" );
                ToRead = *(_8*)buffer;
                LastMessage++;
            }
            else if( (test = ((*(_64*)"MESSAGE!")+1)), (LastMessage == test) )
            {
                Log( "(2)" );
                printf( "Relay Message:%s", buffer );
                LastMessage = 0;
            }
            else if( LastMessage == *(_64*)"MASTERIS" )
            {
                printf( "Game Master is: %s", buffer );
                LastMessage = 0;
                RemoveClient( pc );
            }
            else if( LastMessage == (*(_64*)"USERLIST") )
            {
                Log( "(3)" );
                ToRead = *(_16*)buffer;
                Log1( "User list with size of %d", ToRead );
                LastMessage++;
            }
            else if(  (test = ((*(_64*)"USERLIST")+1) ), (LastMessage == test) )
            {
                char *userlist = (char*)buffer;
                userlist[size] = 0;
                printf( "User List:\n%s", userlist );
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
   printf( "Commands are:\n"
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

int main( int argc, char **argv )
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
   	printf( "Relay was not running?\n" );
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
         	printf( "Requesting active user list from relay.\n" );
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
	   //printf( "Relay found, command issued - waiting for close.\n" );
   }
   // no wait - wait doesn't work right on unix :(
//#ifdef _WIN32
   while( pcControl )
      Sleep(10);
//#endif
}

// Checked in by: $Author: jim $
// $Revision: 1.14 $
// $Log: rcontrol.c,v $
// Revision 1.14  2003/07/25 15:57:44  jim
// Update to make watcom compile happy
//
// Revision 1.13  2003/06/02 19:05:58  jim
// Restored some logging... Added som elogging, handled failures better
//
// Revision 1.12  2003/04/08 19:16:21  jim
// initial commit
//
// Revision 1.11  2002/07/10 22:51:56  panther
// Removed some logging messages, added feature to ./rcontrol memory.
//
// Revision 1.10  2002/04/29 19:52:30  panther
// Added command to force scan winners should something bad happen
//
// Revision 1.9  2002/04/23 17:33:57  panther
// *** empty log message ***
//
// Revision 1.8  2002/04/23 17:16:06  panther
// Wait doesn't work on unix? huh?
//
// Revision 1.7  2002/04/23 16:50:29  panther
// *** empty log message ***
//
// Revision 1.6  2002/04/22 13:35:48  panther
// Fixes for actual Linux usage...
//
// Revision 1.5  2002/04/19 00:49:26  panther
// *** empty log message ***
//
// Revision 1.4  2002/04/19 00:35:49  panther
// First pass protocol updates.
//
// Revision 1.3  2002/04/15 16:26:39  panther
// Added revision tags.
//
