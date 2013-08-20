#include <controls.h>
#include <sharemem.h>
#include <stdio.h>
#include <logging.h>

int bDone, bOkay;

#define LST_FIRST 1000
#define BTN_ADD   1001
#define EDT_NEWTEXT 1002
PCONTROL pcList;



void CPROC AddItem( PTRSZVAL psvEdit, PCONTROL pc )
{
	PCONTROL edit = (PCONTROL)psvEdit;
	char msg[256];
	GetControlText( edit, msg, 256 );
	AddListItem( pcList, msg );
}

int main( void )
{
	PCOMMON pf;
	int i;
        _32 used, free, blocks, freeblocks;
        SetSystemLog( SYSLOG_FILE, stdout );
	//SetBlotMethod( BLOT_MMX );
//#ifdef __STATIC__
	SetControlImageInterface( GetImageInterface() );
	SetControlInterface( GetDisplayInterface() );
//#endif
	GetMemStats( &free, &used, &blocks, &freeblocks );
	printf( WIDE("Mem Stats: %d %d %d %d\n"), used, free, blocks, freeblocks );
	pf = CreateFrame( WIDE("ListBox Test"), 0, 0, 256, 256, 0, NULL );

	pcList = MakeListBox( pf, 0, 5, 5, 140, 150, LST_FIRST );
	AddListItem( pcList, WIDE("One") );
	AddListItem( pcList, WIDE("Four") );
	AddListItem( pcList, WIDE("Nine") );
	AddListItem( pcList, WIDE("Items to add") );
	AddListItem( pcList, WIDE("And a very very very long one") );
	/*
	for( i = 0; i <  100; i++ )
	{
		char item[256];
		sprintf( item, WIDE("Item #%03d"), i );
		AddListItem( pcList, item );
	}
	*/
	MakeButton( pf, 5, 180, 70, 18, BTN_ADD, WIDE("Add Item"), 0, AddItem, 
		(PTRSZVAL)MakeEditControl( pf, 5, 160, 140, 14, EDT_NEWTEXT, WIDE("New Item"), 0 ) );

	AddCommonButtons( pf, &bDone, &bOkay );

	DisplayFrame( pf );

	CommonLoop( &bDone, &bOkay );
	DestroyFrame( &pf );

	GetMemStats( &free, &used, &blocks, &freeblocks );
	printf( WIDE("Mem Stats: %d %d %d %d\n"), used, free, blocks, freeblocks );
	if( used )
	{
		DebugDumpMemFile( WIDE("memory.dump") );
	}
	return 0;
}

// $Log: test7.c,v $
// Revision 1.6  2005/02/09 21:24:50  panther
// Update test to new psi api
//
// Revision 1.5  2004/10/12 23:55:05  d3x0r
// checkpoint
//
// Revision 1.4  2003/03/25 09:37:58  panther
// Fix file tails mangled by CVS logging
//
// Revision 1.3  2003/03/25 08:45:55  panther
// Added CVS logging tag
//
