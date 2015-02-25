//void __stdcall DebugBreak(void);
#include <stdhdrs.h>
#include <stdio.h>
#include <sack_types.h>
//void __stdcall Sleep( _32 time );
#include <sharemem.h>
#include <idle.h>
#include <filesys.h>
#include "controlstruc.h"

PSI_NAMESPACE

typedef struct fileopen_tag {
   TEXTCHAR basepath[280];
	TEXTCHAR currentmask[280];
   TEXTCHAR currentname[280];
   int okay, done;
} FILEOPENDATA;

#define LST_FILES    100
#define TXT_PATHNAME 101


void RedrawTemplate( PTRSZVAL psv )
{
	
}

PCONTROL pLoading;

void CPROC AddFile( PTRSZVAL user, CTEXTSTR pathname, int flags )
{
	PLISTITEM newitem;
//cpg26dec2006 c:\work\sack\src\psilib\fileopen.c(34): Warning! W202: Symbol 'pfod' has been defined, but not referenced
//cpg26dec2006	FILEOPENDATA *pfod = (FILEOPENDATA *)user;
   //xlprintf( LOG_ALWAYS )("file is %s %lx", pathname, flags );
   if( flags & SFF_DRIVE )
   {
      TEXTCHAR buffer[10];
      snprintf( buffer, sizeof( buffer ), WIDE("-%c-"), pathname[0] );
      newitem = AddListItem( pLoading, buffer );
      SetItemData( newitem, (PTRSZVAL)flags );
   }
   else
   {
		CTEXTSTR name = pathrchr( pathname );
      if( name )
         name++;
      else
			name = pathname;
		if( flags & SFF_DIRECTORY )
		{
			TEXTCHAR buffer[256];
			snprintf( buffer, sizeof( buffer ), WIDE("%s/"), name );
			newitem = AddListItem( pLoading, buffer );
			SetItemData( newitem, (PTRSZVAL)flags );

		}
      else
		{
			newitem = AddListItem( pLoading, name );
			SetItemData( newitem, (PTRSZVAL)flags );
		}
   }
}

void LoadList( PCONTROL list, FILEOPENDATA *pfod )
{
   void *stuff = NULL;
   while( pLoading ) Idle();
   pLoading = list;
   // this may do nothing on say a unix system... but we may have
	// drives - perhaps we could alias mount points to drive letters...
   SetControlText( GetNearControl( list, TXT_PATHNAME ), pfod->basepath );
	if( !pfod->basepath[0] )
	{
		ScanDrives( AddFile, (PTRSZVAL)pfod );
	}
	else
	{
      AddFile( (PTRSZVAL)pfod, WIDE(".."), SFF_DIRECTORY );
		while( ScanFiles( pfod->basepath
								, pfod->currentmask
								, &stuff, AddFile, SFF_DIRECTORIES, (PTRSZVAL)pfod ) );
	}
	pLoading = 0;
}

void CPROC FileDouble( PTRSZVAL psv, PCONTROL pc, PLISTITEM hli )
{
   _32 flags;
   TEXTCHAR name[256];
   if( hli )
   {
      flags = GetItemData( hli );
      if( flags & SFF_DRIVE )
      {
         FILEOPENDATA *pfod = (FILEOPENDATA *)psv;
         GetListItemText( hli, name, 256 );
			pfod->basepath[0] = name[1];
         ResetList( pc );
         LoadList( pc, pfod );
      }
      else if( flags & SFF_DIRECTORY )
      {
         //char *newpath;
         FILEOPENDATA *pfod = (FILEOPENDATA *)psv;
         GetListItemText( hli, name, 256 );
         ResetList( pc );
         if( strcmp( name, WIDE("../") ) == 0 )
         {
            TEXTSTR trim = (TEXTSTR)pathrchr( pfod->basepath );
            if( trim )
            {
               if( trim != pfod->basepath &&
                   trim[-1] != ':' )
               {
                  trim[0] = 0; // kill this
               }
               else
                  trim[0] = 0;
				}
				else
				{
               pfod->basepath[0] = 0;
				}
         }
         else
			{
            name[strlen(name)-1] = 0;
            snprintf( pfod->basepath, sizeof(pfod->basepath), WIDE("%s/%s"), pfod->basepath, name );
         }
         LoadList( pc, pfod );
      }
      else
		{
			FILEOPENDATA *pfod = (FILEOPENDATA *)psv;
			//printf( WIDE("setting done?") );
			pfod->okay = TRUE;
      }
   }
}

void CPROC FileSingle( PTRSZVAL psv, PSI_CONTROL list, PLISTITEM hli )
{
	TEXTCHAR name[256];
	if( hli )
	{
		GetListItemText( hli, name, sizeof( name ) );
		SetControlText( GetNearControl( list, TXT_PATHNAME ), name );
	}
}

int PSI_PickFile( PSI_CONTROL parent, CTEXTSTR basepath, CTEXTSTR types, TEXTSTR result, _32 result_len, int Create )
{
	// auto configure open file dialog - well for now let us start at
	// the current working directory...
	S_32 x, y;
	PSI_CONTROL frame;
	PCONTROL pcList;
	FILEOPENDATA fod;

	GetMousePosition( &x, &y );
	frame = CreateFrame( WIDE("Open File"), x, y, 320, 200, BORDER_NORMAL, 0 );
	//char path[280];
	if( !basepath || (strcmp( basepath, WIDE(".") )== 0) )
		GetCurrentPath(fod.basepath, sizeof( fod.basepath ) );
	else
		StrCpyEx( fod.basepath, basepath, sizeof( fod.basepath ) / sizeof(TEXTCHAR) );
	fod.done = 0;
	fod.okay = 0;
	MakeEditControl( frame, 5, 5, 240, 20, TXT_PATHNAME, NULL, 0 );

	//MakeTextControl( frame, 0, 5, 5, 240, 20, TXT_PATHNAME, NULL );
	pcList = MakeListBox( frame, 5, 25, 240, 150, LST_FILES, LISTOPT_SORT );

	SetDoubleClickHandler( pcList, FileDouble, (PTRSZVAL)&fod );
	SetSelChangeHandler( pcList, FileSingle, (PTRSZVAL)&fod );
	StrCpyEx( fod.currentmask, WIDE("*"), sizeof( fod.currentmask ) / sizeof(TEXTCHAR) );
	LoadList( pcList, &fod );

	AddCommonButtons( frame, &fod.done, &fod.okay );
	DisplayFrameOver( frame, parent ); // must be called - even though without it it's still drawn :/
ReLoop:
	CommonWait( frame );
	if( fod.okay )
	{
		// well suppose we should pull out our values before closing this frame...
		PLISTITEM hli = GetSelectedItem( pcList );
		_32 flags = GetItemData( hli );
		if( !(flags & (SFF_DRIVE|SFF_DIRECTORY) ) )
		{
			GetControlText( GetControl( frame, TXT_PATHNAME ), fod.currentname, 280 );
			snprintf( result, result_len, WIDE("%s/%s"), fod.basepath, fod.currentname );
			DestroyCommon( &frame );
			return 1;
		}
		else
		{
			// show - invalid selection - no filename - is a directory...
			//printf( WIDE("Clearing done?") );
			fod.okay = 0;
			FileDouble( (PTRSZVAL)&fod, pcList, hli );
			goto ReLoop;
		}
	}
	DestroyCommon( &frame );
	return 0;
}

int PSI_OpenFile( CTEXTSTR basepath, CTEXTSTR types, TEXTSTR result )
//( PSI_CONTROL parent, CTEXTSTR basepath, CTEXTSTR types, TEXTSTR result, _32 result_len )
{

   return PSI_PickFile( NULL, basepath, types, result, 256, 0 );
}


PSI_NAMESPACE_END

// $Log: fileopen.c,v $
// Revision 1.21  2005/02/18 19:42:38  panther
// fix some update issues with hiding and revealing controls/frames... minor fixes for new API changes
//
// Revision 1.20  2005/02/10 16:55:51  panther
// Fixing warnings...
//
// Revision 1.19  2004/11/29 11:29:53  panther
// Minor code cleanups, investigate incompatible display driver
//
// Revision 1.18  2004/10/24 20:09:47  d3x0r
// Sync to psilib2... stable enough to call it mainstream.
//
// Revision 1.3  2004/10/07 04:37:16  d3x0r
// Okay palette and listbox seem to nearly work... controls draw, now about that mouse... looks like my prior way of cheating is harder to step away from than I thought.
//
// Revision 1.2  2004/10/06 09:52:16  d3x0r
// checkpoint... total conversion... now how does it work?
//
// Revision 1.1  2004/09/19 19:22:31  d3x0r
// Begin version 2 psilib...
//
// Revision 1.17  2003/11/29 00:10:28  panther
// Minor fixes for typecast equation
//
// Revision 1.16  2003/09/22 10:45:08  panther
// Implement tree behavior in standard list control
//
// Revision 1.15  2003/08/27 07:58:39  panther
// Lots of fixes from testing null pointers in listbox, font generation exception protection
//
// Revision 1.14  2003/07/24 23:47:22  panther
// 3rd pass visit of CPROC(cdecl) updates for callbacks/interfaces
//
// Revision 1.13  2003/05/01 21:31:57  panther
// Cleaned up from having moved several methods into frame/control common space
//
// Revision 1.12  2003/04/08 11:41:26  panther
// Don't define a global frame variable in fileopen
//
// Revision 1.11  2003/03/25 08:45:56  panther
// Added CVS logging tag
//
