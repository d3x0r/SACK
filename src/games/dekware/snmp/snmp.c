#include <windows.h>

#include <network.h>
#include <snmp/libsnmp.h>

#include "plugin.h"

int bInitialized;
//   bool InitSnmp( void );
//   bool bRead;
struct snmp_mib_tree *Mib; // current working root of Mib Tree...
//   bool ReadMib( void );
//   int SNMPRead( oid *root, int rootlen, char *buffer );
//   int SNMPWrite( oid *root, int rootlen, int nType, char *buffer );
char GatewayIP[256];
char Community[256];
struct snmp_session	session;
struct snmp_session *ss;
BYTE byTrapMsg[4096];  // really BIG receive buffer...
PCLIENT pcTrap;
struct snmp_pdu      *pdu, *response;

oid root[MAX_NAME_LEN];
int rootlen;

int nResult;

BOOL ReadMib( void )
{
   WIN32_FIND_DATA  fileinfo;
   HANDLE handle;   
   TCHAR FileName[256];
   struct snmp_mib_tree *mib_tree;
   Mib = mib_tree = NULL; // initialize tree..
   GetCurrentPath( FileName, 256 );

   handle = FindFirstFile( "*.mib", &fileinfo );
   if( handle == INVALID_HANDLE_VALUE )
      return FALSE;
   // eventually this should read all .MIB files
   // in the current directory and assemble them
   // all into one big mib tree....
   do
   {
       Mib = mib_tree = read_mib_v2( (char*)fileinfo.cFileName );
       if( mib_tree )
       {
          break;
          // ReadTree( mib_tree );
       }
   } while( !FindNextFile( handle, &fileinfo ) );
   return TRUE;
}

//-------------------------------------------------------------

BOOL InitSnmp( void )
{
   if( ss )
      snmp_close( ss );

    memset((char *)&session, '\0', sizeof(struct snmp_session));
    session.Version   = SNMP_VERSION_1;
    session.peername  = GatewayIP;
    session.community = (u_char*)Community;
    session.community_len = strlen((char *)Community);
    session.retries     = SNMP_DEFAULT_RETRIES;
    session.timeout     = SNMP_DEFAULT_TIMEOUT;
    session.remote_port = SNMP_DEFAULT_REMPORT;

    snmp_synch_setup(&session); // don't call this if we're using a callback
    ss = snmp_open(&session);
    if (ss == NULL){
#ifdef LOG_DEBUG
       OutputDebugString( "Failed Session Creation\n" );
#endif
       bInitialized = FALSE;
       return FALSE;
    }
#ifdef LOG_DEBUG
    OutputDebugString( "Completed Session Setup\n" );
#endif
    bInitialized = TRUE;
    return TRUE;
}


//-------------------------------------------------------------

enum
{
	HELP
}; 
#define NUM_COMMANDS (sizeof(commands)/sizeof(command_entry))
command_entry commands[]={
	{DEFTEXT("HELP")    ,0, 4, DEFTEXT("display help..." ), HELP }
};
int nCommands = NUM_COMMANDS;

//-------------------------------------------------------------


int SNMP( PSENTIENT ps, PTEXT parameters )
{
   int idx;
   PTEXT command;

   if( command = GetParam( ps, &parameters ) ) 
   {
      idx = GetCommandIndex( commands, NUM_COMMANDS
                           , GetTextSize(command), GetText(command) );
      if( idx < 0 )
      {
         DECLTEXT( msg, "Unknown SNMP function specified." );
         EnqueLink( &ps->Command->Output, &msg );
         return 0;
      }
      switch( idx )
      {
      case HELP:
         {
            DECLTEXT( leader, " --- SNMP Builtin Commands ---" );
            EnqueLink( &ps->Command->Output, &leader );
            WriteCommandList( &ps->Command->Output, commands, NUM_COMMANDS, NULL );
         }
         break;
      }
   }
   return 0;
}

// $Log: snmp.c,v $
// Revision 1.3  2003/03/25 09:41:17  panther
// Fix what CVS logging broke...
//
// Revision 1.2  2003/03/25 08:59:03  panther
// Added CVS logging
//
