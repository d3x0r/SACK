
//#include <windows.h>
#include <stdhdrs.h>
#include "fileopen.h"
#include <controls.h>
#include <vidlib.h>
//#undef WIN32

//#ifdef __LINUX__
#define COMPAT_MODE
//#endif

/*

filter == something like "Bodies\0*.Body\0"

*/

//------------------------------------------
#ifndef WIN32
#define HWND int
#define BOOL int
#endif
LOGICAL SelectExistingFile( PSI_CONTROL parent, TEXTCHAR * szFile, _32 buflen, TEXTCHAR * filter )
{
#ifdef COMPAT_MODE
   return PSI_PickFile( parent, WIDE( "." ), filter, szFile, buflen, FALSE );
#else
#ifdef WIN32
   OPENFILENAME ofn;       // common dialog box structurechar szFile[260];       // buffer for filenameHWND hwnd;              // owner windowHANDLE hf;              // file handle// Initialize OPENFILENAMEZeroMemory(&ofn, sizeof(OPENFILENAME));
	char CurPath[256];
   char real_filter[256];
   int filters, n;
	int x;
   HWND hParent = GetNativeHandle( GetFrameRenderer( GetFrame( parent ) ) );
	szFile[0] = 0;
	{
      char *start;
      char *q = real_filter;
		char *p = filter;
		while( p[0] )
		{
         start = p;
			while( p[0] && p[0] != '\t' )
			{
				q[0] = p[0];
				p++;
            q++;
			}
			q[0] = 0;
			if( p[0] ) p++;
			q++;
			p = start;
			while( p[0] && p[0] != '\t' )
			{
				q[0] = p[0];
				p++;
				q++;
			}
			q[0] = 0;
			if( p[0] ) p++;
			q++;
		}
		q[0] = 0;

	}

   memset( &ofn, 0, sizeof( OPENFILENAME ) );
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hParent;
   ofn.lpstrFile = szFile;
   ofn.nMaxFile = 256;
   ofn.lpstrFilter = real_filter;
   ofn.nFilterIndex = 1;
   GetCurrentDirectory( sizeof( CurPath ), CurPath );
   ofn.lpstrInitialDir = CurPath;
   ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST
              | OFN_NOREADONLYRETURN;// Display the Open dialog box. 

   x = GetOpenFileName(&ofn);
	return x;
#endif
#endif
}

//------------------------------------------
LOGICAL SelectNewFile( PSI_CONTROL parent, TEXTCHAR * szFile, _32 buflen, TEXTCHAR * filter )
{
#ifdef COMPAT_MODE
   return PSI_PickFile( parent, WIDE( "." ), filter, szFile, buflen, TRUE );
#else
#ifdef WIN32
   
   OPENFILENAME ofn;       // common dialog box structurechar szFile[260];       // buffer for filenameHWND hwnd;              // owner windowHANDLE hf;              // file handle// Initialize OPENFILENAMEZeroMemory(&ofn, sizeof(OPENFILENAME));
   char CurPath[256];
   char real_filter[256];
   HWND hParent = GetNativeHandle( GetFrameRenderer( GetFrame( parent ) ) );
   szFile[0] = 0;
	{
      char *start;
      char *q = real_filter;
		char *p = filter;
		while( p[0] )
		{
         start = p;
			while( p[0] && p[0] != '\t' )
			{
				q[0] = p[0];
				p++;
            q++;
			}
			q[0] = 0;
			if( p[0] ) p++;
			q++;
         p = start;
			while( p[0] && p[0] != '\t' )
			{
				q[0] = p[0];
				p++;
            q++;
			}
			q[0] = 0;
			if( p[0] ) p++;
			q++;
		}
		q[0] = 0;

	}
   memset( &ofn, 0, sizeof( OPENFILENAME ) );
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = hParent;
   ofn.lpstrFile = szFile;
   ofn.nMaxFile = 256;
   ofn.lpstrFilter = real_filter;
   ofn.nFilterIndex = 1;
   GetCurrentDirectory( sizeof( CurPath ), CurPath );
   ofn.lpstrInitialDir = CurPath;
   ofn.Flags = OFN_NOTESTFILECREATE
              | OFN_NOREADONLYRETURN ;// Display the Open dialog box. 

   return GetSaveFileName(&ofn);
#endif
#endif
}

