#include <stdhdrs.h>
#include <stdio.h>
#include <sharemem.h>
#include <logging.h>
#include <filesys.h>
#include "space.h"

#if defined( WIN32 ) || defined( _WIN32 )
extern int b95;
#endif

//---------------------------------------------------------------------------
// Command Pack 4 deals with Internal Diagnostics, and System
// operating conditions - memdump, changedir....
// save - macros at least for now - should save the universe one day...
//---------------------------------------------------------------------------

int CPROC PRIORITY( PSENTIENT ps, PTEXT parameters )
{
	// Just a note : Thread priority does not work 
	// there are MANY MANY threads involved - so only
	// process priority applies...
#if defined( WIN32 ) || defined( _WIN32 )
	PTEXT pTemp;
   HANDLE hToken, hProcess;
   TOKEN_PRIVILEGES tp;
   OSVERSIONINFO osvi;
   DWORD dwPriority = 0xA5A5A5A5;

   osvi.dwOSVersionInfoSize = sizeof( osvi );
   GetVersionEx( &osvi );
   if( osvi.dwPlatformId  == VER_PLATFORM_WIN32_NT )
   {
      // allow shutdown priviledges....
      // wonder if /shutdown will work wihtout this?
      if( DuplicateHandle( GetCurrentProcess(), GetCurrentProcess()
                        , GetCurrentProcess(), &hProcess, 0
                        , FALSE, DUPLICATE_SAME_ACCESS  ) )
         if( OpenProcessToken( hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken ) )
         {
            tp.PrivilegeCount = 1;
            if( LookupPrivilegeValue( NULL
                                    , SE_SHUTDOWN_NAME
                                    , &tp.Privileges[0].Luid ) )
            {
               tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
               AdjustTokenPrivileges( hToken, FALSE, &tp, 0, NULL, NULL );
            }
            else
               GetLastError();
         }
         else
            GetLastError();
      else
         GetLastError();
   }

	if( ps->CurrentMacro )
      ps->CurrentMacro->state.flags.bSuccess = TRUE;

	pTemp = GetParam( ps, &parameters );
	if( pTemp )
	{
		if( TextLike( pTemp, "idle" ) )
		{
	      dwPriority = IDLE_PRIORITY_CLASS;
		}
		else if( TextLike( pTemp, "normal" ) )
		{
	      dwPriority = NORMAL_PRIORITY_CLASS;
		}
		else if( TextLike( pTemp, "high" ) )
		{
	      dwPriority = HIGH_PRIORITY_CLASS;
		}
		else if( TextLike( pTemp, "realtime" ) )
		{
	      dwPriority = REALTIME_PRIORITY_CLASS;
		}
		else
		{
			if( !ps->CurrentMacro )
			{
				DECLTEXT( msg, "Invalid process priority (not idle, normal, high, or realtime)" );
				EnqueLink( &ps->Command->Output, &msg );
			}
			else
			{
		      ps->CurrentMacro->state.flags.bSuccess = FALSE;
			}
		}
	}
	else
	{
		if( !ps->CurrentMacro )
		{
			DECLTEXT( msg, "No process priority specified (idle,normal,high,realtime)" );
			EnqueLink( &ps->Command->Output, &msg );
		}
		else
		{
		   ps->CurrentMacro->state.flags.bSuccess = FALSE;
		}
	}

	if( dwPriority != 0xA5A5A5A5 )
		SetPriorityClass( GetCurrentProcess(), dwPriority );

	if( !b95 )
	{
		// NT type system we opened these handles
		CloseHandle( hProcess );
		CloseHandle( hToken );
	}
#endif
	return 0;
}

int CPROC MEMDUMP( PSENTIENT ps, PTEXT parameters )
{
	PTEXT pName;
	pName = GetParam( ps, &parameters );
	if( pName )
	{
		DebugDumpMemFile( GetText( pName ) );
	}
	else
	   DebugDumpMem();
	return FALSE;
}
//--------------------------------------
int CPROC MEMORY( PSENTIENT ps, PTEXT parameters )
{
   uint32_t nFree, nUsed, nChunks, nFreeChunks;
	PVARTEXT vt;
	PTEXT temp, next;
   next = GetParam( ps, &parameters );
	while( temp = next )
	{
		if( temp && TextLike( temp, "log" ) )
		{
			temp = GetParam( ps, &parameters );
			if( temp && TextLike( temp, "off" ) )
				SetAllocateLogging( FALSE );
			else
			{
				SetAllocateLogging( TRUE );
				if( temp )
				{
					next = temp;
					continue;
				}
			}
		}
		if( temp && TextLike( temp, "debug" ) )
		{
			temp = GetParam( ps, &parameters );
			if( temp && TextLike( temp, "off" ) )
				SetAllocateDebug( TRUE );
			else
			{
				SetAllocateDebug( FALSE );
				if( temp )
				{
					next = temp;
					continue;
				}
			}
		}
      next = GetParam( ps, &parameters );
	}

	GetMemStats( &nFree, &nUsed, &nChunks, &nFreeChunks );

   vt = VarTextCreate( );
   vtprintf( vt, "Memory : Free %ld(%ld), Used %ld(%ld)  (optional paramters \'log\',\'debug\' followed by off to disable.", nFree, nFreeChunks,
                         nUsed, nChunks - nFreeChunks );
   EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
   VarTextDestroy( &vt );
   return FALSE;
}
int CPROC CMD_BREAK( PSENTIENT ps, PTEXT parameters )
{
#ifdef _WIN32
   DebugBreak();
#endif
//         _asm int 3;
   return FALSE;
}
//--------------------------------------

int CPROC CHANGEDIR( PSENTIENT ps, PTEXT parameters )
{
   PTEXT pDir, pDir1;
   pDir1 = MacroDuplicateEx( ps, parameters, TRUE, TRUE );
   if( pDir1 )
      pDir = BuildLine( pDir1 );
   else
      return FALSE;
   LineRelease( pDir1 );
   if( SetCurrentPath( GetText( pDir ) ) )
   {
      if( ps->CurrentMacro )
         ps->CurrentMacro->state.flags.bSuccess = TRUE;
      else
      {
			DECLTEXT( msg, "Changed directory successfully." );
			{
            TEXTCHAR buf[257];
				GetCurrentPath( buf, sizeof( buf ) );
				SetDefaultFilePath( buf );
			}
         EnqueLink( &ps->Command->Output, &msg );
      }
   }
   else
   {
      if( !ps->CurrentMacro )
      {
         DECLTEXT( msg, "Failed to change directory." );
         EnqueLink( &ps->Command->Output, &msg );
      }
   }
   LineRelease( pDir );

   return FALSE;
}

//---------------------------------------------------------------------------

void StoreMacros( FILE *pFile, PSENTIENT ps )
{

   {
      INDEX idx;
      PLIST pVars;
      PTEXT pVar, pVal;
      pVars = ps->Current->pVars;
      fprintf( pFile, "\n## Begin Variables\n" );
      LIST_FORALL( pVars, idx, PTEXT, pVar )
      {
         fprintf( pFile, "/Declare %s ", GetText( pVar ) );
         pVal = BuildLine( GetIndirect( NEXTLINE( pVar ) ) );
         if( pVal )
            fprintf( pFile, "%s\n", GetText( pVal ) );
         else
            fprintf( pFile, "\n" );
      }
      fprintf( pFile, "\n## Begin Macros\n" );
   }
   {
      PMACRO pm;
      PLIST pMacros = ps->Current->pMacros;
      INDEX idx;
      LIST_FORALL( pMacros, idx, PMACRO, pm )
      {
         PTEXT pParam, pCmd;
         INDEX idx;
         // destroy prefix is unneeded with today's technology
         // and any OLDer versions don't exist - or do in such small number... 
         //fprintf( pFile, "/dest %s\n", GetText( GetName( pm ) ) );
         fprintf( pFile, "/macro %s ", GetText( GetName( pm ) ) );
         pParam = pm->pArgs;
         while( pParam )
         {
            fprintf( pFile, "%s ", GetText( pParam ) );
            pParam = NEXTLINE( pParam );
         }
         fprintf( pFile, "\n" );
         LIST_FORALL( pm->pCommands, idx, PTEXT, pCmd )
         {
            PTEXT pOut; // oh - maintain some sort of order
            pOut = BuildLine( pCmd );
            fprintf( pFile, "%s\n", GetText( pOut ) );
            LineRelease( pOut );
         }
         fprintf( pFile, "\n" );
      }
   }
}

//---------------------------------------------------------------------------

int CPROC SAVE( PSENTIENT ps, PTEXT parameters )
{
         {
            PTEXT pName;
            pName = GetFileName( ps, &parameters );
            if( pName )
            {
               FILE *pFile;
               pFile = sack_fopen( 0, GetText( pName ), "wb" );
               if( pFile )
               {
                  StoreMacros( pFile, ps );
                  fclose( pFile );
                  if( ps->CurrentMacro )
                     ps->CurrentMacro->state.flags.bSuccess = TRUE;
               }
            }
         }
   return FALSE;
}

//-----------------------------------------------------------------------------

int CPROC PLUGIN( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	if( ( temp = GetFileName( ps, &parameters ) ) )
	{
		POINTER pInfo = NULL;
		TEXTSTR base = StrDup( GetText( temp ) );
		TEXTSTR mask = (TEXTSTR)pathrchr( base );
		if( mask )
		{
			mask[0] = 0;
			mask++;
		}
		else
		{
			mask = base;
			base = NULL;
		}
		while( ScanFiles( base, mask, &pInfo, LoadAPlugin, 0, 0 ) );
		if( base ) 
			Release( base );
		else
			Release( mask );
		//LoadPlugin( GetText( temp ), ps, parameters );
	}
	return FALSE;
}

//-----------------------------------------------------------------------------

int CPROC UNLOAD( PSENTIENT ps, PTEXT parameters )
{
	PTEXT name = NULL, collapsedname;
	PTEXT temp;
	if( !parameters )
	{
		DumpLoadedPluginList( ps );
		return 0;
	}

   while( ( temp = GetParam( ps, &parameters ) ) )
   {
   	Log2( "Unload name part: %s(%d)"
   			, GetText( temp )
   			, temp->format.position.offset.spaces ) ;
   	if( temp->format.position.offset.spaces )
   	{
   		if( name )
   		{
   			name->format.position.offset.spaces = 0;
   			collapsedname = BuildLine( name );
   			Log1( "Unload: \'%s\'", GetText( collapsedname ) );
		      Unload( collapsedname );
   			LineRelease( name );
   			LineRelease( collapsedname );
   		}
   		else
	   		name=SegAppend( name, SegDuplicate( temp ) );
   	}	
   	else
   	{
   		name=SegAppend( name, SegDuplicate( temp ) );
   	}
   }
   if( name )
   {
   	name->format.position.offset.spaces = 0;
   	collapsedname = BuildLine( name );
   	Log1( "Unload: \'%s\'", GetText( collapsedname ) );
		Unload( collapsedname );
   	LineRelease( name );
   	LineRelease( collapsedname );
   }
   return FALSE;
}

//-----------------------------------------------------------------------------

int CPROC STORY( PSENTIENT ps, PTEXT parameters )
{
   Story( &ps->Command->Output );
	return FALSE;
}

//-----------------------------------------------------------------------------

int CPROC DUMP( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	PVARTEXT vt;
	while( parameters )
	{
		PENTITY pe;
		PSENTIENT peS;
		vt = VarTextCreate( );
		//if( temp == ps->Current->pName )
		//{
		//	pe = ps->Current;
	   //   vtprintf( vt, "%s is you.", GetText(temp) );
	   //}
		//else
		{
			enum FindWhere foundat;

			PTEXT pName = NULL;
         //PTEXT pParam;
			if( !(pe = (PENTITY)FindThingEx( ps, &parameters, ps->Current, FIND_GRABBABLE, &foundat, &pName, NULL DBG_SRC ) )
			  &&!(pe = (PENTITY)FindThingEx( ps, &parameters, ps->Current, FIND_AROUND, &foundat, &pName, NULL DBG_SRC)) )
			{
				temp = GetParam( ps, &parameters );
				if( temp == ps->Current->pName )
				{
					pe = ps->Current;
               vtprintf( vt, "%s is you.", GetText( temp ) );
				}
				else
				{
					vtprintf( vt, "Cannot find %s.", GetText(temp));
					EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
					return 0;
				}
			}
			else
			{
				switch( foundat )
				{
				case FIND_ON:
	            vtprintf(vt,"%s held by you.", GetText(GetName( pe )) );
	            break;
				case FIND_IN:
	            vtprintf(vt,"%s carried by you.", GetText(GetName( pe )) );
					break;
				case FIND_NEAR:
	            vtprintf(vt,"%s is in this room.", GetText(GetName( pe )) );
					break;
				case FIND_AROUND:
	            vtprintf(vt,"%s is your room.", GetText(GetName( pe )) );
					break;
				}
				// now what info would be useful to dump about this
				// object...
			}
	  	}
	  	if( pe )
	  	{
   		EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
			if( ( peS = pe->pControlledBy ) )
			{
		      vtprintf( vt, "Entity is aware." );
   		   EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
				vtprintf( vt, "Flags:%s%s%s%s%s%s"
						  , peS->ProcessLock?" PLOCK":""
						  , peS->StepLock?" SLOCK":""
						  , peS->pRecord?" Recording":""
						  , peS->flags.macro_input?" input":""
						  , peS->flags.destroy?" destroy":""
						  , peS->flags.bRelay?" relay":""
						  );
   		   EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
			}
			if( peS )
			{
				PMACROSTATE pms;
				int n;
				if( peS->pRecord )
				{
		  			vtprintf( vt, "Recording macro: %s", GetText( GetName( peS->pRecord ) ) );
	   		   		EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
		  		}
		  		for( n = 0; (pms = (PMACROSTATE)PeekDataEx( &peS->MacroStack, n )); n++ )
		  		{
		  			if( !n )
		  			{
			  			vtprintf( vt, "Macro stack..." );
						EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
		  			}
					if( pms->peInvokedOn == ps->Current )
					{
	               if( pms->pMacro->pDescription )
		            {
			            vtprintf( vt, "[%s] - %s"
							        , GetText( GetName( pms->pMacro ) )
							        , GetText( pms->pMacro->pDescription ) );
						}
	               else
		            {
			            vtprintf( vt, "[%s] -"
							        , GetText( GetName( pms->pMacro ) ) );
					   }
					}
					else
					{
	               if( pms->pMacro->pDescription )
		            {
			            vtprintf( vt, "[%s](%s) - %s"
							        , GetText( GetName( pms->pMacro ) )
									  , GetText( GetName( pms->peInvokedOn ) )
							        , GetText( pms->pMacro->pDescription ) );
						}
	               else
		            {
						if( pms->peInvokedOn )
							vtprintf( vt, "[%s](%s) -"
							        , GetText( GetName( pms->pMacro ) ) 
									  , GetText( GetName( pms->peInvokedOn ) )
									  );
						else
							vtprintf( vt, "[%s] -"
							        , GetText( GetName( pms->pMacro ) ) 
									  );

						}
					}
	   		   EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
		  		}
			}
		  	if( peS )
		  	{
	  			PDATAPATH pdp;
	  			CTEXTSTR pName;
	  			vtprintf( vt, "Command path: object" );
	  			pdp = peS->Command;
				while( pdp )
				{
				 	pName = GetRegisteredValue( (CTEXTSTR)pdp->pDeviceRoot, "Name" );//FindDeviceName( pdp->Type );
				 	if( pName )
				 	{
						vtprintf( vt, "->%s%s", GetText( pdp->pName )
										 , pdp->flags.Closed?"(closed)":"" );
					}
					else
						vtprintf( vt, "->UNKNOWN" );
					pdp = pdp->pPrior;
				}
   			EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
				if( peS->Data )
				{
			  		vtprintf( vt, "Data path: object" );
			  		pdp = peS->Data;
					while( pdp )
					{
				 		pName = GetRegisteredValue( (CTEXTSTR)pdp->pDeviceRoot, "Name" );//FindDeviceName( pdp->Type );
					 	if( pName )
					 	{
							vtprintf( vt, "->%s[%s]%s", GetText( pdp->pName ), pName
												    , pdp->flags.Closed?"(closed)":"" );
						}
						else
							vtprintf( vt, "->UNKNOWN" );
						pdp = pdp->pPrior;
					}
				}
				else
		  			vtprintf( vt, "Data path: None" );
   			EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
		  	}
	  	}
	  	VarTextDestroy( &vt );
	}
	return FALSE;
}

//-----------------------------------------------------------------------------

void WriteTextFlags( PSENTIENT ps, PTEXT pSeg )
{
	PVARTEXT vt;
	vt = VarTextCreate( );
	vtprintf( vt, "Text Flags: ");
	if( pSeg->flags & TF_STATIC )
		vtprintf( vt, "static " );
	if( pSeg->flags & TF_QUOTE )
		vtprintf( vt, WIDE("\"\" ") );
	if( pSeg->flags & TF_SQUOTE )
		vtprintf( vt, "\'\' " );
	if( pSeg->flags & TF_BRACKET )
		vtprintf( vt, "[] " );
	if( pSeg->flags & TF_BRACE )
		vtprintf( vt, "{} " );
	if( pSeg->flags & TF_PAREN )
		vtprintf( vt, "() " );
	if( pSeg->flags & TF_TAG )
		vtprintf( vt, "<> " );
	if( pSeg->flags & TF_INDIRECT )
		vtprintf( vt, "Indirect " );
   /*
	if( pSeg->flags & TF_SINGLE )
	vtprintf( vt, "single " );
   */
	if( pSeg->flags & TF_FORMATABS )
      vtprintf( vt, "format x,y " );
   else
		vtprintf( vt, "format spaces(%d) ", pSeg->format.position.offset.spaces );
	if( pSeg->flags & TF_COMPLETE )
		vtprintf( vt, "complete " );
	if( pSeg->flags & TF_BINARY )
		vtprintf( vt, "binary " );
	if( pSeg->flags & TF_DEEP )
		vtprintf( vt, "deep " );
	if( pSeg->flags & TF_ENTITY )
		vtprintf( vt, "entity " );
	if( pSeg->flags & TF_SENTIENT )
		vtprintf( vt, "sentient " );
	if( pSeg->flags & TF_NORETURN )
		vtprintf( vt, "NoReturn " );
	if( pSeg->flags & TF_LOWER )
		vtprintf( vt, "Lower " );
	if( pSeg->flags & TF_UPPER )
		vtprintf( vt, "Upper " );
	if( pSeg->flags & TF_EQUAL )
		vtprintf( vt, "Equal " );
	if( pSeg->flags & TF_TEMP )
		vtprintf( vt, "Temp " );
	if( pSeg->flags & TF_PROMPT )
		vtprintf( vt, "Prompt " );
	if( pSeg->flags & TF_PLUGIN )
		vtprintf( vt, "Plugin=%02x ", (uint8_t)(( pSeg->flags >> 24 ) & 0xff) );
	EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
	VarTextDestroy( &vt );
}

//-----------------------------------------------------------------------------

void DumpSegment( PSENTIENT ps, PTEXT pSeg, int bSingle )
{
	while( pSeg )
	{
		WriteTextFlags( ps, pSeg );
		if( pSeg->flags & TF_INDIRECT )
		{
			DECLTEXT( msg, "------ Indirect Content ------");
			EnqueLink( &ps->Command->Output, &msg );

			DumpSegment( ps, GetIndirect( pSeg ), FALSE /*&pSeg->flags & TF_SINGLE */);
		}
		else
		{
			PVARTEXT vt;
			vt = VarTextCreate( );
			vtprintf( vt, "(%ld)%s"
							 , pSeg->data.size
							 , pSeg->data.data 
					  );
			EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
			VarTextDestroy( &vt );
		}	
		if( !bSingle )
			pSeg = NEXTLINE( pSeg );
		else
			break;
	} 
}


//-----------------------------------------------------------------------------


int CPROC DUMPVAR( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp, var;
	DECLTEXT( msg3, "------- End Segment Dump --------" );
	while( temp = parameters, var = GetParam( ps, &parameters ) )
	{
		if( temp == var )
		{
			PVARTEXT vt;
			vt = VarTextCreate( );
			vtprintf( vt, "%s is a simple text element.", GetText( temp ) );
			EnqueLink( &ps->Command->Output, VarTextGet( vt ) );
			VarTextDestroy( &vt );
		}
		else // pretty safe to assume that this will be a variable entity.
		{
			DECLTEXT( msg1, "----------- Segment ------------" );
			DECLTEXT( msg2, "----------- Contains -----------" );
			EnqueLink( &ps->Command->Output, &msg1 );
			DumpSegment( ps, temp, TRUE );
			EnqueLink( &ps->Command->Output, &msg2 );
			DumpSegment( ps, var, FALSE );
		}

	}
	EnqueLink( &ps->Command->Output, &msg3 );
	return FALSE;
}
// $Log: syscmds.c,v $
// Revision 1.24  2005/04/24 10:00:21  d3x0r
// Many fixes to finding things, etc.  Also fixed resuming on /wait command, previously fell back to the 5 second poll on sentients, instead of when data became available re-evaluating for a command.
//
// Revision 1.23  2005/04/20 06:20:25  d3x0r
//
// Revision 1.22  2005/01/28 16:02:20  d3x0r
// Clean memory allocs a bit, add debug/logging option to /memory command,...
//
// Revision 1.21  2004/09/24 08:06:11  d3x0r
// Restore the ability to generate a breakpoint in win32 compilers
//
// Revision 1.20  2004/01/20 07:38:06  d3x0r
// Extended dumped flags
//
// Revision 1.19  2003/12/10 21:59:51  panther
// Remove all LIST_ENDFORALL
//
// Revision 1.18  2003/11/07 23:45:28  panther
// Port to new vartext abstraction
//
// Revision 1.17  2003/08/15 13:19:12  panther
// Device subsystem much more robust?
//
// Revision 1.16  2003/03/25 08:59:03  panther
// Added CVS logging
//
