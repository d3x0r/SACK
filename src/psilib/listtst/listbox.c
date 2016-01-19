#define DEFINE_DEFAULT_IMAGE_INTERFACE
#include <stdhdrs.h>
#include <psi.h>

int bDone, bOkay;

#define LST_FIRST 1000
#define BTN_ADD   1001
#define EDT_NEWTEXT 1002
#define LST_TREE  1003
#define TXT_TEST 1004
PCONTROL pcList, pcTree, pcText;



void CPROC AddItem( PTRSZVAL psvEdit, PCONTROL pc )
{
	PCONTROL edit = (PCONTROL)psvEdit;
	TEXTCHAR msg[256];
	GetControlText( edit, msg, 256 );
	AddListItem( GetNearControl( pc, LST_FIRST ), msg );
	SetControlText( pcText, msg );
}



PSI_CONTROL CreateListTester( PSI_CONTROL parent )
{
	int n;
	_32 used, free, blocks, freeblocks;
   	PCOMMON pf;
	GetMemStats( &free, &used, &blocks, &freeblocks );

	printf( WIDE("Mem Stats: %") _32f WIDE(" %") _32f WIDE(" %") _32f WIDE(" %") _32f WIDE("\n")
			, used, free, blocks, freeblocks );

	for( n = 0; n < 1; n++ )
	{
	pf = CreateFrame( WIDE("ListBox Test"), 0, 0, 400, 256, 0, parent );
	lprintf( WIDE("To make list...") );
	pcList = MakeListBox( pf, 5, 5, 140, 150, LST_FIRST, 0 );
	AddListItem( pcList, WIDE("One") );
	AddListItem( pcList, WIDE("Four") );
	AddListItem( pcList, WIDE("Nine") );
	AddListItem( pcList, WIDE("Items to add") );
	AddListItem( pcList, WIDE("And a very very very long one") );

   SetCommonTransparent( pcList, TRUE );
   SetCommonTransparent( GetFirstChildControl( pcList ), TRUE );

   SetControlColor( GetFirstChildControl( GetFirstChildControl( pcList ) ), NORMAL, AColor( 128, 0, 128, 32 ) );
   SetControlColor( GetNextControl( GetFirstChildControl( GetFirstChildControl( pcList ) ) ), NORMAL, AColor( 128, 0, 128, 32 ) );
   SetControlColor( GetFirstChildControl( pcList ), NORMAL, AColor( 128, 128, 128, 32 ) );
   SetControlColor( GetFirstChildControl( pcList ), SCROLLBAR_BACK, AColor( 128, 128, 128, 32 ) );
   SetControlColor( pcList, EDIT_BACKGROUND, AColor( 128, 128, 128, 32 ) );
	/*
	for( i = 0; i <  100; i++ )
	{
		char item[256];
		sprintf( item, WIDE("Item #%03d"), i );
		AddListItem( pcList, item );
	}
	*/
	MakeButton( pf, 5, 180, 70, 18, BTN_ADD, WIDE("Add Item"), 0, AddItem,
				  (PTRSZVAL)MakeEditControl( pf, 5, 160, 140, 16, EDT_NEWTEXT, WIDE("New Item"), 0 ) );
   pcText = MakeTextControl( pf, 5, 203, 140, 18, TXT_TEST, WIDE("text to change"), 0 );
   pcTree = MakeListBox( pf, 150, 5, 200, 150, LST_TREE, LISTOPT_TREE );
   SetListboxIsTree( pcTree, TRUE );
   AddListItemEx( pcTree, 0, WIDE("tree top") );
   AddListItemEx( pcTree, 1, WIDE("one") );
   AddListItemEx( pcTree, 1, WIDE("one") );
   AddListItemEx( pcTree, 2, WIDE("two tree top") );
   AddListItemEx( pcTree, 3, WIDE("three tree top") );
   AddListItemEx( pcTree, 1, WIDE("one tree top") );
   AddListItemEx( pcTree, 0, WIDE("0 tree top") );
   AddListItemEx( pcTree, 1, WIDE("one tree top") );
   AddListItemEx( pcTree, 2, WIDE("two tree top") );
   AddListItemEx( pcTree, 3, WIDE("three tree top") );

	AddCommonButtons( pf, &bDone, &bOkay );
	if( parent )
		DisplayFrameOver( pf, parent );
   else
		DisplayFrame( pf );
}
//	DestroyFrame( pf );

	GetMemStats( &free, &used, &blocks, &freeblocks );
	printf( WIDE("Mem Stats: %") _32f WIDE(" %") _32f WIDE(" %") _32f WIDE(" %") _32f WIDE("\n"), used, free, blocks, freeblocks );
	if( used )
	{
		DebugDumpMemFile( WIDE("memory.dump") );
	}
   return pf;
}

SaneWinMain( argc, argv )
{
	CommonWait( CreateListTester( CreateListTester( NULL ) ) );
	//SetSystemLog( SYSLOG_NONE, 0 );
	return 0;
}
EndSaneWinMain()

// $Log: listbox.c,v $
// Revision 1.23  2005/05/17 18:37:33  jim
// remove noisy logging.
//
// Revision 1.22  2005/04/13 18:46:36  jim
// Use default system logging...
//
// Revision 1.21  2005/04/06 18:32:48  panther
// Added a text control that changes... to demonstrate that this function works.
//
// Revision 1.20  2005/02/09 21:23:44  panther
// Update macros and function definitions to follow the common MakeControl parameter ordering.
//
// Revision 1.19  2004/10/31 17:22:28  d3x0r
// Minor fixes to control library...
//
// Revision 1.18  2004/10/12 23:55:10  d3x0r
// checkpoint
//
// Revision 1.17  2004/10/08 15:24:05  d3x0r
// Got buttons updating, and listboxes... and fairly everything which itself updates... but need to check for all dirty related controls also...
//
// Revision 1.16  2004/09/28 22:13:59  d3x0r
// Implement CommonWait(pf) which does a permanent sleep if not needing to idle.
//
// Revision 1.15  2004/08/16 07:13:17  d3x0r
// Make 5 list test boxes (display debug)
//
// Revision 1.14  2004/03/23 17:02:44  d3x0r
// Use common interface library to load video/image interface
//
// Revision 1.13  2003/10/13 02:50:47  panther
// Font's don't seem to work - lots of logging added back in
// display does work - but only if 0,0 biased, cause the SDL layer sucks.
//
// Revision 1.12  2003/10/12 02:47:05  panther
// Cleaned up most var-arg stack abuse ARM seems to work.
//
// Revision 1.11  2003/10/07 20:29:49  panther
// Modify render to accept flags, test program to test bits.  Generate multi-bit alpha
//
// Revision 1.10  2003/09/22 10:45:08  panther
// Implement tree behavior in standard list control
//
// Revision 1.9  2003/08/20 08:07:13  panther
// some fixes to blot scaled... fixed to makefiles test projects... fixes to export containters lib funcs
//
// Revision 1.8  2003/07/24 23:47:22  panther
// 3rd pass visit of CPROC(cdecl) updates for callbacks/interfaces
//
// Revision 1.7  2003/05/02 01:10:22  panther
// OKay - fixed list test - gets the right values...
//
// Revision 1.6  2003/03/25 09:37:58  panther
// Fix file tails mangled by CVS logging
//
// Revision 1.5  2003/03/25 08:45:57  panther
// Added CVS logging tag
//
