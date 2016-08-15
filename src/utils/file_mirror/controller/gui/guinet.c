#include <stdhdrs.h>
#include <stdio.h>
#include <sharemem.h>

#include <network.h>
#include <controls.h>

#include <logging.h>
#include <idle.h>
#include "resources.h"

extern PCOMMON frame;

#define NL_MYSELF 0

int InProgress;

PCLIENT *ppc_current;

void BasicMessageBox( TEXTCHAR *title, TEXTCHAR *content )
{
   	PCOMMON msg;
TEXTCHAR *start, *end;
        TEXTCHAR msgtext[256];
   	int done = 0, okay = 0;
   	int y = 5;
   	msg = CreateFrame( title, 0, 0, 312, 120, 0, frame );
   	end = start = content;
   	do
   	{
	   	while( end[0] && end[0] != '\n' )
   			end++;
   		if( end[0] )
                {
                    MemCpy( msgtext, start, end-start );
                    msgtext[end-start] = 0;
   			//end[0] = 0;
	   		MakeTextControl( msg, 5, y, 302, 16, -1
   						, msgtext, 0 );
	   	   //end[0] = '\n';
   		   end = start = end+1;
   		   y += 18;
	   	}
	   	else
	   		MakeTextControl( msg, 5, y, 302, 16, -1
   						, start, 0 );
	   } while( end[0] );
		//AddExitButton( msg, &done );
		AddCommonButtons( msg, NULL, &okay );
		DisplayFrame( msg );
		CommonLoop( &okay, NULL );
		DestroyFrame( &msg );
}

void CPROC ConnectionClose(PCLIENT pc)
{
   if( ppc_current )
		(*ppc_current) = NULL;
	InProgress = FALSE;
}

void CPROC ReadComplete( PCLIENT pc, POINTER buffer, size_t size )
{
	static uint64_t LastMessage; // this is safe - only ONE connection EVER
	uint64_t test;
	int ToRead = 8;
	if( !buffer )
	{
		Log( WIDE("Initial Read issued allocated  buffer...read events a go.") );
		buffer = Allocate( 1024 );
	}
	else
	{
     	((TEXTCHAR*)buffer)[size] = 0;
     	if( !LastMessage )
     	{
			Log1( WIDE("Message is: %8.8s"), buffer );
			if( *(uint64_t*)buffer == *(uint64_t*)"USERLIST" )
			{
				Log1( WIDE("Message is: USERLIST!!"), buffer );
				ToRead = 2;
				LastMessage = *(uint64_t*)buffer;
				Log1( WIDE("Last Message is: %8.8s"), &LastMessage );
			}
			else if( *(uint64_t*)buffer == *(uint64_t*)"USERDEAD" )
			{
				BasicMessageBox( WIDE("Relay Responce"), WIDE("User has been terminated!") );
				RemoveClient( pc );
			}
			else if( *(uint64_t*)buffer == *(uint64_t*)"ALL DONE" )
			{
				RemoveClient( pc );
			}
			else if( *(uint64_t*)buffer == *(uint64_t*)"MESSAGE!" ||
			         *(uint64_t*)buffer == *(uint64_t*)"WINNERS:" )
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
				printf( WIDE("Unknown responce from relay: %8.8s"), buffer );
			}
		}
		else
		{
			Log1( WIDE("Continuing message: %8.8s"), &LastMessage );
			if( LastMessage == *(uint64_t*)"MESSAGE!" ||
			    LastMessage == *(uint64_t*)"WINNERS:" )
			{
				Log( WIDE("(1)") );
				ToRead = *(uint8_t*)buffer;
				LastMessage++;
			}
         else if( (test = ((*(uint64_t*)"WINNERS:")+1)), (LastMessage == test) )
         {
				PCONTROL pcList = GetControl( frame, LST_WINNERS );
				TEXTCHAR *winnerlist = (TEXTCHAR*)buffer;
				TEXTCHAR *endline, lastchar;
				ResetList( pcList );
				winnerlist[size] = 0;
				Log2( WIDE("Got %d bytes of data:%s"), size, winnerlist );
				endline = winnerlist;
				do
				{
					while( endline[0] && ( endline[0] != ',' && endline[0] != ':' ) )
					{
						if( endline[0] == 3 )
							endline[0] = ' '; // space fill this...
						endline++;
					}
					lastchar = endline[0];
				   //if( endline[0] )
			   	{
						endline[0] = 0;
						AddListItem( pcList, winnerlist );
						if( lastchar )
						{
					   	winnerlist = endline+1;
					   	while( winnerlist[0] && winnerlist[0] == ' ' ) winnerlist++;
					   	endline = winnerlist;
					   }
				   }
				} while( endline[0] );
				LastMessage = 0;
         }
			else if( (test = ((*(uint64_t*)"MESSAGE!")+1)), (LastMessage == test) )
			{
				Log( WIDE("(2)") );
				BasicMessageBox( WIDE("Relay Message"), DupCharToText( (char*)buffer ) );
				LastMessage = 0;
			}
			else if( LastMessage == *(uint64_t*)"MASTERIS" )
			{
				if( *(uint64_t*)buffer == *(uint64_t*)"ABSENT.." )
				{
					SetControlText( GetControl( frame, CHK_MASTER ), WIDE("Game Master is absent") );
				}
				else if( *(uint64_t*)buffer == *(uint64_t*)"PRESENT!" )
				{
					SetControlText( GetControl( frame, CHK_MASTER ), WIDE("Game Master is PRESENT") );
				}
				else
				{
					BasicMessageBox( WIDE("Master Status"), WIDE("Unknown Responce...") );
				}
				LastMessage = 0;
				RemoveClient( pc );
			}
			else if( LastMessage == (*(uint64_t*)"USERLIST") )
			{
				Log( WIDE("(3)") );
				ToRead = *(uint16_t*)buffer;
				Log1( WIDE("User list with size of %d"), ToRead );
				LastMessage++;
			}
			else if(  (test = ((*(uint64_t*)"USERLIST")+1) ), (LastMessage == test) )
			{
				PCONTROL pcList = GetControl( frame, LST_USERS );
				TEXTCHAR *userlist = (TEXTCHAR*)buffer;
				TEXTCHAR *endline;
				//Log1( WIDE("Got %d bytes of data..."), size );
				ResetList( pcList );
				userlist[size] = 0;
				endline = userlist;
				do
				{
					while( endline[0] && endline[0] != '\n' )
					{
						if( endline[0] == 3 )
							endline[0] = ' '; // space fill this...
						endline++;
					}
				   if( endline[0] )
			   	{
			   		TEXTCHAR *endname;
			   		TEXTCHAR *realname;
			   		PLISTITEM hli;
						endline[0] = 0;
						hli = AddListItem( pcList, userlist );
						endname = userlist;
						while( endname[0] != ' ' )
							endname++;
						endname[0] = 0;
						realname = NewArray( TEXTCHAR, ( endname - userlist ) + 1 );
						StrCpyEx( realname, userlist, max( ( endname - userlist ), size ) );
						SetItemData( hli, (uintptr_t)realname );
			   		endline++;
				   	userlist = endline;
				   }
				} while( endline[0] );
				LastMessage = 0;
				RemoveClient( pc );
			}
			else
			{
				Log1( WIDE("Unknonw Continuation state for %8.8s"), &LastMessage );
			}
		}
	}
	ReadTCPMsg( pc, buffer, ToRead );
}



PCLIENT ConnectToRelay( PCLIENT *pc )
{
	TEXTCHAR address[128];
	if( InProgress )
	{
		BasicMessageBox( WIDE("Request in progress"), WIDE("Please wait a moment for\n")
																 WIDE("the current operation to finish") );		
		return NULL;
	}
	InProgress = TRUE;
	NetworkWait( NULL, 5, 16 );
	GetControlText( GetControl( frame, EDT_SERVER ), address, sizeof( address ) );

	*pc = OpenTCPClientEx( address, 3001, ReadComplete, ConnectionClose, NULL );
   if( !(*pc) )
   {
   		BasicMessageBox( WIDE("Bad Address?"), 
                       WIDE("Check the address - could not connect") );
		return NULL;
	}
   ppc_current = pc;
   {
	   FILE *file = sack_fopen( 0, WIDE("LastConnection.data"), WIDE("wb") );
	   if( file )
	   {
	   		fprintf( file, WIDE("%s"), address );
   			fclose( file );
   		}
   }
   return *pc;
}


void CPROC GetUserButton(uintptr_t psv, PCONTROL pcButton)
{
	PCLIENT pc;
	ConnectToRelay( &pc );
	if( pc )
	{
		SendTCP( pc, "LISTUSER", 8 );
		while( pc )
         Idle();
	}
}

void CPROC KillRelayButton(uintptr_t psv, PCONTROL pcButton)
{
	PCLIENT pc;
	int i = 0;
	ConnectToRelay( &pc );
	if( pc )
	{
		SendTCP( pc, "DIE NOW!", 8 );
		while( pc )
		{
			Sleep(1000);
			SendTCP( pc, "DIE NOW!", 8 );
			i++;
			if( i == 5 )
			{
				BasicMessageBox( WIDE("Kill failed"), WIDE("Relay has not responded by ending connection") );
				RemoveClient( pc );
			}
		}
	}
}

void CPROC KillUserButton(uintptr_t psv, PCONTROL pcButton)
{
	PCLIENT pc;
	ConnectToRelay( &pc );
	if( pc )
	{
		char msg[256];
		int len;
		len = sprintf( msg, "KILLUSER%s",
					GetItemData(
						GetSelectedItem(
							GetControl( frame, LST_USERS ) ) ) );
		SendTCP( pc, msg, len );
		while( pc )
			Idle();
	}
}

void CPROC RebootUserButton(uintptr_t psv, PCONTROL pcButton)
{
	PCLIENT pc;
	ConnectToRelay( &pc );
	if( pc )
	{
		char msg[256];
		int len;
		len = sprintf( msg, "REBOOT!!%s",
					GetItemData(
						GetSelectedItem(
							GetControl( frame, LST_USERS ) ) ) );
		SendTCP( pc, msg, len );
		while( pc )
			Idle();
	}
}

void CPROC ScanUserButton(uintptr_t psv, PCONTROL pcButton)
{
	PCLIENT pc;
	ConnectToRelay( &pc );
	if( pc )
	{
		char msg[256];
		int len;
		len = sprintf( msg, "DO SCAN!%s",
					GetItemData(
						GetSelectedItem(
							GetControl( frame, LST_USERS ) ) ) );
		SendTCP( pc, msg, len );
		while( pc )
			Idle();
	}
}

void CPROC UpdateUserButton(uintptr_t psv, PCONTROL pcButton)
{
	PCLIENT pc;
	ConnectToRelay( &pc );
	if( pc )
	{
		char msg[256];
		int len;
		len = sprintf( msg, "UPDATE  %s",
					GetItemData(
						GetSelectedItem(
							GetControl( frame, LST_USERS ) ) ) );
		SendTCP( pc, msg, len );
		while( pc )
			Idle();
	}
}

void CPROC ScanAllButton(uintptr_t psv, PCONTROL pcButton)
{
	PCLIENT pc;
	ConnectToRelay( &pc );
	if( pc )
	{
		SendTCP( pc, "DO SCAN!", 8 );
		while( pc )
		{
			ProcessControlMessages();
			Sleep(0);
		}
	}
}

void CPROC GetMasterStatusButton(uintptr_t psv, PCONTROL pcButton)
{
	PCLIENT pc;
	ConnectToRelay( &pc );
	if( pc )
	{
		SendTCP( pc, "MASTER??", 8 );
		while( pc )
         Idle();
	}
}


void CPROC UpdateAllButton(uintptr_t psv, PCONTROL pcButton)
{
	PCLIENT pc;
	ConnectToRelay( &pc );
	if( pc )
	{
		SendTCP( pc, "UPDATE  ", 8 );
		while( pc )
         Idle();
	}
}

void CPROC RequestWinnersButton(uintptr_t psv, PCONTROL pcButton)
{
	PCLIENT pc;
	ConnectToRelay( &pc );
	if( pc )
	{
		SendTCP( pc, "WINNERS?", 8 );
		while( pc )
         Idle();
	}
}

// Checked in by: $Author: jim $
// $Revision: 1.17 $
// $Log: guinet.c,v $
// Revision 1.17  2003/09/29 15:44:39  jim
// Updates for new network mods
//
// Revision 1.16  2003/07/25 15:57:44  jim
// Update to make watcom compile happy
//
// Revision 1.15  2003/05/06 00:00:45  jim
// Update for new psi control interface
//
// Revision 1.14  2003/02/19 00:29:46  jim
// Port to newest libraries and makesystem
//
// Revision 1.13  2002/05/22 19:00:58  panther
// *** empty log message ***
//
// Revision 1.12  2002/05/22 18:58:34  panther
// *** empty log message ***
//
// Revision 1.11  2002/05/14 21:08:41  panther
// Added holdoff message box
//
// Revision 1.10  2002/05/03 15:43:11  panther
// Updates to get winners report from relay master.
//
// Revision 1.9  2002/04/29 15:07:10  panther
// Fixed game master present/absent status control to something more
// usable than that stupid check box.
//
// Revision 1.8  2002/04/24 17:41:49  panther
// CLOSE the connection after MASTER status request...
//
// Revision 1.7  2002/04/23 16:45:37  panther
// *** empty log message ***
//
// Revision 1.6  2002/04/15 16:26:39  panther
// Added revision tags.
//
