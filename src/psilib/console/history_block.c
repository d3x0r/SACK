
#include <stdhdrs.h>
#include <filesys.h>
#include <logging.h>
#include <psi/console.h>
#include <sqlgetoption.h>
#include "regaccess.h"

#include "consolestruc.h" // all relavent includes
#include "history.h"
#include "histstruct.h"

PSI_CONSOLE_NAMESPACE

//----------------------------------------------------------------------------

PHISTORYBLOCK PSI_DestroyRawHistoryBlock( PHISTORYBLOCK pHistory )
{
	int32_t i;
	PHISTORYBLOCK next;
	for( i = 0; i < pHistory->nLinesUsed; i++ )
	{
		LineRelease( pHistory->pLines[i].pLine );
	}
	if( ( next = ( (*pHistory->me) = pHistory->next ) ) )
		pHistory->next->me = pHistory->me;
	Release( pHistory );
	return next;
}


//----------------------------------------------------------------------------

PHISTORYBLOCK CreateRawHistoryBlock( void )
{
	PHISTORYBLOCK pHistory;
	pHistory = (PHISTORYBLOCK)Allocate( sizeof( HISTORYBLOCK ) );
	MemSet( pHistory, 0, sizeof( HISTORYBLOCK ) );
	return pHistory;
}

//----------------------------------------------------------------------------

PHISTORYBLOCK CreateHistoryBlock( PHISTORY_BLOCK_LINK phbl )
{
	PHISTORYBLOCK pHistory;
	// phbl needs to be valid node link thingy...
	if( !phbl->me )
	{
		lprintf( "FATAL ERROR, NODE in list is not initialized correctly." );
		DebugBreak();
	}
	pHistory = CreateRawHistoryBlock();
	/* this is link next... (first in list)
	pHistory->next = phbl->next;
	if( phbl->me ) // in most cases it's 'last'
		(*(pHistory->me = &(phbl->me->next))) = pHistory;
		phbl->next = pHistory;
		*/
	// if this is the head of the list... then that
	// me points at the last node of the list, the
	// next of the last node is NULL, which if we
	// get the next, and set it here will be NULL.

	// if this points at an element within the list,
	// then phbl->me is the node prior to the node passed,
	// and that will be phbl itself, which will set
	// next to be phbl, and therefore will be immediately
	// before the next.
	//pHistory->next = (*phbl->me);
	pHistory->next = (*(pHistory->me = phbl->me));

	// set what is pointing at the passed node, and
	// make my reference of prior node's pointer mine,
	//pHistory->me = phbl->me;
	// and then set that pointer to ME, instead of phbl...
	(*pHistory->me) = pHistory; // set pointer pointing at me to ME
	// finally, phbl's me is now my next, since the new node points at it.
	phbl->me = &pHistory->next;

	return pHistory;
}

//----------------------------------------------------------------------------

void DumpBlock( PHISTORYBLOCK pBlock DBG_PASS )
{
	int idx;
	_xlprintf( 0 DBG_RELAY )("History block used lines: %" _size_f " of %d", pBlock->nLinesUsed, MAX_HISTORY_LINES );
	for( idx = 0; idx < pBlock->nLinesUsed; idx++ )
	{
		PTEXTLINE ptl = pBlock->pLines + idx;
		_xlprintf( 0 DBG_RELAY )("line: %" _size_f " = (%d,%" _size_f ",%s)", idx, ptl->flags.nLineLength, GetTextSize( ptl->pLine ), GetText( ptl->pLine ) );
	}
}




PSI_CONSOLE_NAMESPACE_END
