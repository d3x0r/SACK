#ifndef CORE_SOURCE
#define CORE_SOURCE
#endif

#include "stdhdrs.h"
#include "logging.h"

#include "space.h"

//---------------------------------------------------------------------------
// Data command module includes open/close datapath
//  assignment of command path
// and get data methods from the datapath which is open
//---------------------------------------------------------------------------

void CloseSomeDatapath( PSENTIENT ps, PTEXT parameters, PDATAPATH *root )
{
	int datasource;
	PTEXT temp = GetParam( ps, &parameters );
	if( temp )
	{
		PDATAPATH pdp = FindOpenDevice( ps, temp );
		if( pdp )
			DestroyDataPath( pdp );
	}
	else
	{
		datasource = 1;
		do
		{
			if( root )
			{
				if( (*root) )
				{
					datasource = (*root)->flags.Data_Source;
					DestroyDataPath( (*root) );
				}
				else
					datasource = 1;
			}
		} while( !datasource );
	}
}

//---------------------------------------------------------------------------

int CPROC CMD_ENDPARSE( PSENTIENT ps, PTEXT parameters )
{
	CloseSomeDatapath( ps, parameters, &ps->Data );
	return FALSE;
}
//--------------------------------------

int CPROC FILTER( PSENTIENT ps, PTEXT parameters )
{
	// /filter <command/data> <add/del/insert> <name> <insert-<after>> <options>
	// man this is a pretty complex command - totally does not fit
	// the simplicity - of course if it's gotta be done......
	// /filter command add tee <object>

	return FALSE;
}

//--------------------------------------
int CPROC PARSE( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	PTEXT devname;
	if( ( devname = GetParam( ps, &parameters ) ) )
	{
	// find device to see if it's already open...
	// by name!
	// but - this causes a problem for nested scripts
	// which are going to have to be artificiailly inceminated
	// with a unique name maybe something like script_#####
		PDATAPATH pdp = FindOpenDevice( ps, devname );
		if( pdp )
		{
			if( !ps->CurrentMacro )
			{
				PVARTEXT pvt;
				pvt = VarTextCreate();
				vtprintf( pvt, WIDE("Device named %s is already open."), GetText( devname ) );
				EnqueLink( &ps->Command->Output, (POINTER)VarTextGet( pvt ) );
				VarTextDestroy( &pvt );
			}
			return -1;
		}
		if( ( temp = GetParam( ps, &parameters ) ) )
		{
			PDATAPATH pdp;
			lprintf( WIDE("Opening device handle named: %s"), GetText( devname ) );
			if( ( pdp = OpenDevice( &ps->Data, ps, temp, parameters ) ) )
			{
				while( devname->flags & TF_INDIRECT )
					devname = GetIndirect( devname );
				pdp->pName = SegDuplicate( devname );
				if( pdp->Type )
				{
					if( ps->CurrentMacro )
					{
						ps->CurrentMacro->state.flags.bSuccess = TRUE;
					}
				}
				else
				{
					DestroyDataPath( pdp ); // new one did not finish open...
				}
			}
		}
		else
		{
			DECLTEXT( msg, WIDE("Parse must specify a name (/(parse/open) <name> <device> <options...)") );
			EnqueLink( &ps->Command->Output, (PTEXT)&msg );
		}
	}
	return FALSE;
}

//--------------------------------------

int CPROC CMD_ENDCOMMAND( PSENTIENT ps, PTEXT parameters )
{
	CloseSomeDatapath( ps, parameters, &ps->Command );
	return FALSE;
}

//--------------------------------------
int CPROC COMMAND( PSENTIENT ps, PTEXT parameters )
{
	PTEXT temp;
	 PTEXT devname;
	 if( ( devname = GetParam( ps, &parameters ) ) )
	 {
		  // find device to see if it's already open...
		  // by name! 
		  // but - this causes a problem for nested scripts
		  // which are going to have to be artificiailly inceminated
		  // with a unique name maybe something like script_#####
		  PDATAPATH pdp = FindOpenDevice( ps, devname );
		  if( pdp )
		  {
			if( !ps->CurrentMacro )
			{
					 PVARTEXT pvt;
					 pvt = VarTextCreate();
					 vtprintf( pvt, WIDE("Device named %s is already open."), GetText( devname ) );
					 EnqueLink( &ps->Command->Output, (POINTER)VarTextGet( pvt ) );
					 VarTextDestroy( &pvt );
				}
				return -1;
		  }
		 if( ( temp = GetParam( ps, &parameters ) ) )
		  {
			  PDATAPATH pdp;

			  //lprintf( WIDE("Opening device handle named: %s"), GetText( devname ) );
			  if( ( pdp = OpenDevice( &ps->Command, ps, temp, parameters ) ) )
			  {
				  PTEXT pCommand;
				  PDATAPATH pOldPath;
				  while( devname->flags & TF_INDIRECT )
					  devname = GetIndirect( devname );
				  pdp->pName = SegDuplicate( devname );
				  if( ps->Command )
				  {
				  // remove old command queue so close won't remove
				  // ps->Command->Input = NULL;
				  // close the command queue...

				  // update to the new queue in the sentience
				  // if the new one is a data source, then we
				  // need to move old commands to new path -
				  // otherwise they may/will get lost.
					  if( ps->Command->flags.Data_Source
						 && !ps->Command->flags.Dont_Relay_Prior)
								 {
									 pOldPath = ps->Command->pPrior;

												 // move all commands from old queue to new queue
									 while( ( pCommand = (PTEXT)DequeLink( &pOldPath->Input ) ) )
									 {
										 EnqueLink( &ps->Command->Input, pCommand );
									 }

									 while( ( pCommand = (PTEXT)DequeLink( &pOldPath->Output ) ) )
									 {
										 EnqueLink( &ps->Command->Output, pCommand );
									 }
								 }
				  }
				  if( ps->CurrentMacro )
					  ps->CurrentMacro->state.flags.bSuccess = TRUE;
			  }
		  }
		  else
		  {
				DECLTEXT( msg, WIDE("Command must specify a name (/command <name> <device> <options...)") );
				EnqueLink( &ps->Command->Output, (PTEXT)&msg );
		  }
	}
	return FALSE;
}
//--------------------------------------
	// exit closes the command datapath...
int CPROC EXIT( PSENTIENT ps, PTEXT parameters ) // should check to save work...
{
			if( ps->Data )
			{
				// uhmm yeah maybe...
				// is actually a KILL command...
				DestroyDataPath( ps->Data );
				ps->Data = NULL;
			}

			if( ps->Command &&
				 ps->Command->Type )
			{
				DestroyDataPath( ps->Command );
				ps->Command = NULL;
			}
			else
			{
				ExitNexus();
			}
	 return TRUE;
}
//--------------------------------------
int CPROC CMD_GETPARTIAL( PSENTIENT ps, PTEXT parameters )
{
	if( ps->Data )
	{
		PTEXT pSave, temp;
		pSave = parameters;
		if( ( temp = GetParam( ps, &parameters ) ) )
		{
			if( temp == pSave )
			{
				DECLTEXT( msg, WIDE("Parameter to GetPartial was not a variable reference.") );
				EnqueLink( &ps->Command->Output, &msg );
				return FALSE;
			}

			if( GetIndirect( temp ) )
			{
				LineRelease( GetIndirect( temp ) );
				SetIndirect( temp, NULL );
			}

			if( ps->Data->Partial )
			{
				SetIndirect( temp, ps->Data->Partial );
				ps->Data->Partial = NULL;
				if( ps->CurrentMacro )
					ps->CurrentMacro->state.flags.bSuccess = TRUE;
			}
		}
	}
	 return FALSE;
}
//--------------------------------------

int GetInputData( int bWord, PSENTIENT ps, PTEXT parameters )
{
	if( ps->Data )
	{
		PTEXT pSave, temp;
		PTEXT *pInd = NULL;
		pSave = parameters;
		lprintf( WIDE("Get data from datapath...") );
		if( !( ( temp = GetParam( ps, &parameters ) ) ) )
		{
			lprintf( WIDE("No parameters specified to get line") );
			LineRelease( ps->pLastResult );
			ps->pLastResult = NULL;
			pInd = &ps->pLastResult;
		}
		else
		{
			if( temp == pSave )
			{
				DECLTEXT( msg, WIDE("is not a valid variable reference.") );
				EnqueLink( &ps->Command->Output, SegAppend( SegDuplicate( temp ), (PTEXT)&msg ) );
				return FALSE;
			}
			if( temp->flags & TF_INDIRECT )
			{
				LineRelease( GetIndirect( temp ) );
				SetIndirect( temp, NULL );
				pInd = (PTEXT*)&temp->data.size;
			}
			else
			{
				DECLTEXT( msg, WIDE("is not a valid variable reference(not indirect).") );
				EnqueLink( &ps->Command->Output, SegAppend( SegDuplicate( temp ), (PTEXT)&msg ) );
				return FALSE;
			}
		}

		if( !pInd )
		{
			DECLTEXT( msg, WIDE("FATAL ERROR: Did not set pInd (GetLine)") );
			EnqueLink( &ps->Command->Output, (PTEXT)&msg );
				return FALSE;
		  }
		// try grabbing an existing line - could be put there from BURST of prior...

		if( !ps->Data->CurrentWord ) // just got last word off line...
		{
			if( ps->Data->CurrentLine )
			{
				LineRelease( ps->Data->CurrentLine );
				ps->Data->CurrentLine = NULL;
			}
		}

		if( !ps->Data->CurrentLine )
		{
			 // Gather Data from input source....
		 // data from HT_CLIENT 'appears' asynchrous
		  // to scripting....
			if( ps->Data->Read )
				ps->Data->Read( ps->Data ); // this returns a completeline...
			ps->Data->CurrentLine = (PTEXT)DequeLink( &ps->Data->Input );
		}

		if( ps->Data->CurrentLine )
			if( !ps->Data->CurrentWord )
				ps->Data->CurrentWord = ps->Data->CurrentLine;

		if( ps->Data->CurrentWord )
		{
			// indirect pointer will be null.
			if( bWord )
			{
				*pInd = SegDuplicate( ps->Data->CurrentWord );
				ps->Data->CurrentWord = NEXTLINE( ps->Data->CurrentWord );
			}
			else
			{
				// duplicate to end of line... which may be whole - or
				// may be partial because of prior GETWORD from the line...
				// then release the data input line...
				// must duplicate the data in either case cause
				// it will be deleted with additional setting of the
				// variable...
					 *pInd = TextDuplicate( ps->Data->CurrentWord, FALSE );
				LineRelease( ps->Data->CurrentWord );
				ps->Data->CurrentLine = NULL; // do NOT release this line...
				ps->Data->CurrentWord = NULL; // no word on line
			}
		}
		if( ps->CurrentMacro && *pInd )
			ps->CurrentMacro->state.flags.bSuccess = TRUE;
	}
	 return FALSE;
}

//--------------------------------------------------------------------------

int CPROC CMD_GETWORD( PSENTIENT ps, PTEXT parameters )
{
	 return GetInputData( TRUE, ps, parameters );
}

//--------------------------------------------------------------------------

int CPROC CMD_GETLINE( PSENTIENT ps, PTEXT parameters )
{
	 return GetInputData( FALSE, ps, parameters );
}

//--------------------------------------------------------------------------

CORE_PROC( PDATAPATH, FindDataDatapath )( PSENTIENT ps, int type )
{
	 PDATAPATH pdp = ps->Data;
	 while( pdp )
	 {
		  if( pdp->Type == type )
				break;
		  pdp = pdp->pPrior;
	 }
	 return pdp;
}

//--------------------------------------------------------------------------

CORE_PROC( PDATAPATH, FindCommandDatapath )( PSENTIENT ps, int type )
{
	 PDATAPATH pdp = ps->Command;
	 while( pdp )
	 {
		  if( pdp->Type == type )
				break;
		  pdp = pdp->pPrior;
	 }
	 return pdp;
}

//--------------------------------------------------------------------------

CORE_PROC( PDATAPATH, FindDatapath )( PSENTIENT ps, int type )
{
	 PDATAPATH pdp;
	 pdp = FindDataDatapath( ps, type );
	 if( pdp )
		  return pdp;
	 pdp = FindCommandDatapath( ps, type );
	return pdp;
}

//--------------------------------------------------------------------------

CORE_PROC(int, RelayInput)( PDATAPATH pdp
												, PTEXT (CPROC *Datacallback)( PDATAPATH pdp, PTEXT pLine ) )
{
	extern int gbTrace;
	if( pdp->pPrior )
	{
		PTEXT p;
		int moved = 0;

		if( gbTrace )
			xlprintf(LOG_NOISE+1)( WIDE("Relay input from %s to %s..........")
										, GetText( pdp->pPrior->pName )
										, GetText( pdp->pName ) );
		do
		{
			if( pdp->pPrior->Read )
			{
				pdp->pPrior->Read( pdp->pPrior );
			}
			if( IsQueueEmpty( &pdp->pPrior->Input ) )
				break;
			//Log( WIDE("Has some input to handle...") );
			while( ( p = (PTEXT)DequeLink( &pdp->pPrior->Input ) ) )
			{
				if( gbTrace )
					lprintf( WIDE("Data is: %s"), GetText( p ) );
				if( Datacallback && !( ( p->flags & TF_RELAY ) == TF_RELAY ) )
				{
					//lprintf( WIDE("Data callback...") );
					for( p = Datacallback( pdp, p ); p; p = Datacallback( pdp, NULL ) )
					{
						moved++;
						if( p != (POINTER)1 )
							EnqueLink( &pdp->Input, p );
						else
							lprintf( WIDE("Data was consumed by datapath.") );
					}
				}
				else
				{
					 PTEXT out = BuildLine( p );
					 Log1( WIDE("Relay In: %s"), GetText( out ) );
					 LineRelease( out );
					moved++;
					EnqueLink( &pdp->Input, p );
				}
			}
			if( gbTrace && !moved && !pdp->pPrior->flags.Closed )
			{
				lprintf( WIDE("Did not result in data, try reading source again, rerelay. %s %s"), GetText( pdp->pName ), GetText( pdp->pPrior->pName ) );
			}
		  } while( !moved &&
					 !pdp->pPrior->flags.Closed );
		// stop relaying closes at datasource points
		//if( !pdp->pPrior->flags.Data_Source )
		pdp->flags.Closed |= pdp->pPrior->flags.Closed;
		return moved;
	}
	return 0;
}

//--------------------------------------------------------------------------

CORE_PROC(int, RelayOutput)( PDATAPATH pdp
									, PTEXT (CPROC *Datacallback)( PDATAPATH pdp, PTEXT pLine ) )
{
	extern int gbTrace;
	if( pdp->pPrior )
	{
		int moved = 0;
		PTEXT p;
		//if( gbTrace )
		//	lprintf( WIDE("Relay output from %s to %s..........")
		//			 , GetText( pdp->pName )
		//			 , GetText( pdp->pPrior->pName )
		//			 );
		while( ( p = (PTEXT)DequeLink( &pdp->Output ) ) )
		{
			//if( gbTrace )
			//	lprintf( WIDE("Data is: %s"), GetText( p ) );
			if( Datacallback && !( ( p->flags & TF_RELAY ) == TF_RELAY ) )
			{
				for( p = Datacallback( pdp, p ); p; p = Datacallback( pdp, NULL ) )
				{
					EnqueLink( &pdp->pPrior->Output, p );
				}
			}
			else
			{
#ifdef _DEBUG
				//PTEXT out = BuildLine( p );
				//Log1( WIDE("Relay Out: %s"), GetText( out ) );
				//LineRelease( out );
#endif
				EnqueLink( &pdp->pPrior->Output, p );
			}
		}
		// stop relaying closes at datasource points
		//if( !pdp->pPrior->flags.Data_Source )
		pdp->flags.Closed |= pdp->pPrior->flags.Closed;
		if( pdp->pPrior->Write )
			moved = pdp->pPrior->Write( pdp->pPrior );
		return moved;
	}
	else
	{
		PTEXT p;
		while( ( p = (PTEXT)DequeLink( &pdp->Output ) ) )
		{
			if( gbTrace )
			{
				PTEXT pLine = BuildLine( p );
				lprintf( WIDE("Relay to dev NULL... Data is: %s"), GetText( pLine ) );
				LineRelease( pLine );
			}
			LineRelease( p );
		}
	}
	return 0;
}

//--------------------------------------------------------------------------
// $Log: datacmds.c,v $
// Revision 1.28  2005/05/30 11:51:42  d3x0r
// Remove many messages...
//
// Revision 1.27  2005/04/25 07:56:55  d3x0r
// Fix lingering data on NULL path datapaths.  Also, return find_self as a result of searching... there is only one object that will match %me... and that object, although not really 'grabbable' or 'visilbe' is both.
//
// Revision 1.26  2005/04/24 10:00:20  d3x0r
// Many fixes to finding things, etc.  Also fixed resuming on /wait command, previously fell back to the 5 second poll on sentients, instead of when data became available re-evaluating for a command.
//
// Revision 1.25  2005/02/21 12:08:55  d3x0r
// Okay so finally we should make everything CPROC type pointers... have for a long time decided to avoid this, but for compatibility with other libraries this really needs to be done.  Increase version number to 2.4
//
// Revision 1.24  2005/01/17 09:01:17  d3x0r
// checkpoint ...
//
// Revision 1.23  2004/09/27 16:06:56  d3x0r
// Checkpoint - all seems well.
//
// Revision 1.22  2004/05/04 07:29:50  d3x0r
// Massive reformatting.  Fixed looping, removed some gotos. Implement the relay for TF_RELAY segments.
//
// Revision 1.21  2004/03/07 17:50:48  d3x0r
// Fixed lost event when data passing through trigger during read invokes a macro
//
// Revision 1.20  2003/10/29 02:56:16  panther
// Misc fixes with base filters.  Data throughpaths...
//
// Revision 1.19  2003/10/29 02:11:51  panther
// Fix relay... fix reading data path, and close final
//
// Revision 1.18  2003/10/26 12:37:20  panther
// Rework event scheduling/handling based on newest Heavy Sleep Mechanism
//
// Revision 1.17  2003/08/20 08:05:48  panther
// Fix for watcom build, file usage, stuff...
//
// Revision 1.16  2003/08/15 13:19:12  panther
// Device subsystem much more robust?
//
// Revision 1.15  2003/04/06 23:26:13  panther
// Update to new SegSplit.  Handle new formatting option (delete chars) Issue current window size to launched pty program
//
// Revision 1.14  2003/03/25 08:59:03  panther
// Added CVS logging
//
