#include <stdhdrs.h>

#include "rcomlib.h"
#include "vidlib.h"
#include "plugin.h"

#include "resource.h"

PENTITY pTray;
PSENTIENT psTray;

extern HWND hWndDialog;

void QueueCommand( char *Command )
{
   PTEXT pc, pb;
   pc = SegCreateFromText( Command );
   pb = burst( NULL, pc );
   EnqueLink( psTray->Command->ppInput, pb );
   LineRelease( pc );
}

char LastTime[25]; // xx/xx/xxxx xx:xx:xx
char Zipcode[25]; // xxxxx

char Browser[MAX_PATH];
char BrowserOpt[256];
char BannerLink[1024]="localhost";

#define BANNERTIMER 100
#define REPORTTIMER 101


char CmdLine[4096];
void MouseHandler( DWORD dwUser, int x, int y, int b )
{
   static int _b;
   if( b & MK_LBUTTON && !( _b & MK_LBUTTON ) ) 
   {
      STARTUPINFO si;
      PROCESS_INFORMATION  pi;
		memset( &si, 0, sizeof( si ) );
		si.cb = sizeof( STARTUPINFO );
		sprintf( CmdLine, "%s %s", BrowserOpt, BannerLink );
      CreateProcess( Browser, CmdLine, NULL, NULL, FALSE, 0, 
                     NULL, NULL, &si, &pi );
   }
   _b = b;
}

BOOL CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   switch( uMsg )
   {
   case WM_INITDIALOG:
      SetTimer( hWnd, BANNERTIMER, 120000, NULL ); // 
      SetTimer( hWnd, REPORTTIMER, 120000, NULL ); // 

      if( !GetRegistryString( "Weather Tray", "Last Date", LastTime, 25 ) )
      {
         // never requested anything...
         SYSTEMTIME st;
         GetSystemTime( &st );
         sprintf( LastTime, "%02d/%02d/%d %02d:%02d:%02d", 
                        st.wMonth, st.wDay, st.wYear,
                        st.wHour, st.wMinute, st.wSecond );
         SetRegistryString( "Weather Tray", "Last Date", LastTime );
      }
      if( !GetRegistryString( "Weather Tray", "Local Zipcode", Zipcode, 25 ) )
      {
         SimpleQuery( "Please enter your zip code", Zipcode, 25 );
         SetRegistryString(  "Weather Tray", "Local Zipcode", Zipcode );
         //SimpleQuery( );
      }
      {
         HVIDEO hVidOut;
         hVidOut = GetVideoHandle( hWnd, IMG_BANNER );
         SetMouseHandler( hVidOut, MouseHandler, (DWORD)hVidOut );
      }
      QueueCommand( "/decl image" ); // actual banner image data...
      QueueCommand( "/decl imagelink" );
      QueueCommand( "/decl link" );

      QueueCommand( "/Macro GetBanner" );
      QueueCommand( "/http get Imagedata mail.town.com/ads/ads.asp" );
		QueueCommand( "/if success" );
      QueueCommand( "/decl linkdata" );

      QueueCommand( "/tail %Imagedata %linkdata" );
      QueueCommand( "/if fail" );
      QueueCommand( "/goto EndNow" );
      QueueCommand( "/endif" );
      QueueCommand( "/burst %linkdata banner" ); 

      QueueCommand( "/label loop1" );
      QueueCommand( "   /tail %banner %link" );
      QueueCommand( "   /if fail" );
      QueueCommand( "      /goto EndNow" );
      QueueCommand( "   /endif" );
      QueueCommand( "   /compare %link eol" );
      QueueCommand( "   /if success" );
      QueueCommand( "      /goto loop1" );
      QueueCommand( "   /endif" );
      QueueCommand( "   /compare %link is_quote" );
      QueueCommand( "   /if fail" );
      QueueCommand( "      /goto loop1" );
      QueueCommand( "   /endif" );
      QueueCommand( "   /label loop2" );
      QueueCommand( "   /tail %banner %imagelink" );
      QueueCommand( "   /if fail" );
      QueueCommand( "      /goto EndNow" );
      QueueCommand( "   /endif" );
      QueueCommand( "   /compare %imagelink eol" );
      QueueCommand( "   /if success" );
      QueueCommand( "      /goto loop2" );
      QueueCommand( "   /endif" );
      QueueCommand( "   /compare %imagelink is_quote" );
      QueueCommand( "   /if fail" );
      QueueCommand( "      /goto loop2" );
      QueueCommand( "   /endif" );
      //QueueCommand( "/tell MOOSE /echo Hey Image is at %imagelink" );
      QueueCommand( "/http get image %imagelink" );
      QueueCommand( "/DisplayImage" );
      QueueCommand( "/label EndNow" );
		QueueCommand( "/endif" );
      QueueCommand( "/endmac" );

      // might create another object ... though this should 
      // be able to sit in this loop and add the /getbanner macro
      // only when the timer ticks...
      QueueCommand( "/create Reciever" );
      QueueCommand( "/wake Reciever" );
      QueueCommand( "/tell Reciever /parse udpserver 51718" );
      QueueCommand( "//decl junk" );
      QueueCommand( "//mac idleloop" ); // should receive UDP messages...
      QueueCommand( "//label top" );
      QueueCommand( "//wait" );
      QueueCommand( "//getword %%junk" );
      QueueCommand( "//goto top" );
      QueueCommand( "//endmac" );

      QueueCommand( "/parse udp panther.greater.net:51717" );
      {
         char byCmd[256];
         sprintf( byCmd, "/decl zip %s", Zipcode );
         QueueCommand( byCmd );
      }
      //QueueCommand( "
      //QueueCommand( "
      //QueueCommand( "

      PostMessage( hWnd, WM_TIMER, BANNERTIMER, 0 );
      break;
   case WM_TIMER:
      switch( wParam )
      {
      case BANNERTIMER:
         QueueCommand( "/GetBanner" );
         break;
      case REPORTTIMER:
         QueueCommand( "/send %zip %now" );
         break;
      }
      // QueueCommand( "/http get image localhost/nexus/nexus.jpg" );
      break;   
   case WM_COMMAND:
      switch( LOWORD( wParam ) )
      {
      case IDCANCEL:
         DestroyWindow( hWnd );
         break;
      }
      break;
   case WM_DESTROY:
      ExitNexus(); // this is supposed to be a solo ap...
      break;
   }
   return FALSE;
}

int DisplayImage( PSENTIENT ps, PTEXT parameters )
{
   PTEXT pImage, pLink;
   ImageFile *image;

   pLink = GetVariable( ps->Current->pVars, "link" );
   pLink = GetIndirect( pLink );
   sprintf( BannerLink, "%s", GetText( pLink ) );

   pImage = GetVariable( ps->Current->pVars, "image" );
   pImage = GetIndirect( pImage );

   while( NEXTLINE( pImage ) )
      pImage = NEXTLINE( pImage );
   if( pImage )
   {
      if( !(pImage->flags & TF_BINARY ) )
      {
         MessageBox( NULL, "Blah, so badly screwed...", "Internal page request error", MB_OK );
         return 0;
      }
      else
      {
         image = DecodeMemoryToImage( GetText( pImage ), GetTextSize( pImage ) );
         if( image )
         {
            HVIDEO hVidOut;
            FlipImage( image );
            hVidOut = GetVideoHandle( hWndDialog, IMG_BANNER );
            BlotScaledImageTo( GetImage( hVidOut ), image );
            UpdateVideo( hVidOut );
            UnmakeImageFile( image ); // no longer needed after updated...
         }
      }
   }
   return 0;
}

// $Log: tray.c,v $
// Revision 1.3  2003/03/25 09:41:17  panther
// Fix what CVS logging broke...
//
// Revision 1.2  2003/03/25 08:59:04  panther
// Added CVS logging
//
