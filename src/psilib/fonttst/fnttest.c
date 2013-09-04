
#define USE_IMAGE_INTERFACE GetImageInterface()
#define USE_RENDER_INTERFACE GetRenderInterface()

#undef SACK_BAG_EXPORTS
#undef BAG_EXTERNALS
#include <stdhdrs.h>
#include <filesys.h>
#include <logging.h>
#include <sharemem.h>
#include <stdio.h>
#include <controls.h>
#include <timers.h>

#include <image.h>
#include <render.h>



//-------------------------------------------------------------------------

PTRSZVAL psvFont;
static int CPROC DrawFont( PCOMMON renderer )
{
	SFTFont font = (SFTFont)psvFont;
	Image image = GetFrameSurface( renderer );
	_32 w, h;
	GetStringSizeFont( WIDE(" "), &w, &h, font );
	ClearImageTo( image, BASE_COLOR_WHITE );
	PutStringFont( image, 5, 5, BASE_COLOR_BLACK, 0, WIDE("ABCDEFGHIJKLM"), font );
	PutStringFont( image, 5, 5+h, BASE_COLOR_BLACK, 0, WIDE("NOPQRSTUVWXYZ"), font );
	PutStringFont( image, 5, 5+2*h, BASE_COLOR_BLACK, 0, WIDE("abcdefghijklm"), font );
	PutStringFont( image, 5, 5+3*h, BASE_COLOR_BLACK, 0, WIDE("nopqrstuvwxyz"), font );
	PutStringFont( image, 5, 5+4*h, BASE_COLOR_BLACK, 0, WIDE("01234567890()"), font );
	PutStringFont( image, 5, 5+5*h, BASE_COLOR_BLACK, 0, WIDE("!@#$%^&*,.`{}"), font );

	GetImageSize( image, &w, &h );
	PutStringFont( image, w/2, h/2, BASE_COLOR_BLACK, 0, WIDE("ABCDEFGHIJKLM"), font );
	PutStringInvertFont( image, w/2, h/2, BASE_COLOR_RED, 0, WIDE("ABCDEFGHIJKLM"), font );
	PutStringVerticalFont( image, w/2, h/2, BASE_COLOR_BLUE, 0, WIDE("ABCDEFGHIJKLM"), font );
	PutStringInvertVerticalFont( image, w/2, h/2, BASE_COLOR_GREEN, 0, WIDE("ABCDEFGHIJKLM"), font );
   return 1;
   //UpdateDisplay( renderer );
}

//-------------------------------------------------------------------------

int main( int argc, char **argv )
{
	SFTFont font;
	POINTER fontdata = NULL;
	size_t fontsize = 0;
	if( argc < 6 )
	{
		if( (argc > 1) && (strcmp( DupCharToText( argv[1] ), WIDE("pick") ) == 0) )
		{
			FILE *in;
			in = fopen( WIDE("fonttst.dat"), WIDE("rb") );
			if( in )
			{
				fseek( in, 0, SEEK_END );
				fontsize = ftell( in );
				fontdata = Allocate( fontsize );
				fseek( in, 0, SEEK_SET );
				fread( fontdata, 1, fontsize, in );
				fclose( in );
			}
			font = PickFont( 0, 0, &fontsize, &fontdata, NULL/*pAbove*/ );
			{
				FILE *out;
				out = fopen( WIDE("fonttst.dat"), WIDE("wb") );
				if( out )
				{
					fwrite( fontdata, 1, fontsize, out );
					fclose( out );
				}
			}
         // yeah - you can use dumpfontfile to produce a header which is a font...
			DumpFontFile( argv[2]?argv[2]:"fonttst.dump.h", font );
		}
		else
		{
			printf( WIDE("Usage: %s (outname) (fontfile) (width) (height) (bits)\n"), DupCharToText( argv[0] ) );
			printf( WIDE("rendering lucon.ttf 14x14 into lucidaconsole.h") );
			{
				FILE *in;
				in = fopen( WIDE("fonttst.dat"), WIDE("rb") );
				if( in )
				{
					fseek( in, 0, SEEK_END );
					fontsize = ftell( in );
					fontdata = Allocate( fontsize );
					fseek( in, 0, SEEK_SET );
					fread( fontdata, 1, fontsize, in );
					fclose( in );
				}
			}
			if( fontdata )
				font = RenderFontData( (PFONTDATA)fontdata );
			else
				font = NULL;
			if( !font )
			{
				printf( WIDE("Wow nothing defaulted ever, defaulting to lucon.ttf 14x14") );
				font = RenderFontFile( WIDE("lucon.ttf"), 14, 14, 2 );
			}
         // yeah - you can use dumpfontfile to produce a header which is a font...
			DumpFontFile( WIDE("lucidaconsole.h"), font );
		}
	}
	else 
	{
		int bits = atoi( DupCharToText( argv[5] ) );
		printf( WIDE("Using parameters specified from command line ... %s %sx%s %s bits"), DupCharToText( argv[2] ), DupCharToText( argv[3] ), DupCharToText( argv[4] ), DupCharToText( argv[5] ) );
		if( bits == 8 )
			bits = 3;
		else if( bits == 2 )
			bits = 2;
		else
			bits = 0;
		font = RenderFontFileEx( argv[2], atoi( argv[3] ), atoi( argv[4] ), bits, &fontsize, &fontdata );
		{
			FILE *out;
			out = fopen( WIDE("fonttst.dat"), WIDE("wb") );
			if( out )
			{
				fwrite( fontdata, 1, fontsize, out );
				fclose( out );
			}
		}
		// yeah - you can use dumpfontfile to produce a header which is a font...
		DumpFontFile( argv[1], font );
	}
	psvFont = (PTRSZVAL)font;
	{
		PCOMMON pRend = CreateFrame( WIDE("Demo Font"), 10, 10, 300, 300, BORDER_NORMAL, NULL );
		AddCommonDraw( pRend, DrawFont );
		//DrawFont( (PTRSZVAL)font, pRend );
		DisplayFrame( pRend );
		while( 1 )
			Sleep( 1000 );
	}


	PickFont( 0, 0, 0, NULL, NULL );
	
	//DebugDumpMemFile( WIDE("memory.dump") );
	return 0;
}
// $Log: fnttest.c,v $
// Revision 1.21  2005/03/30 03:26:37  panther
// Checkpoint on stabilizing display projects, and the exiting thereof
//
// Revision 1.20  2005/03/23 02:43:07  panther
// Okay probably a couple more badly initialized 'unusable' flags.. but font rendering/picking seems to work again.
//
// Revision 1.19  2004/12/13 11:06:56  panther
// Some fixes to image button configuration, some better handling of PressButton method
//
// Revision 1.18  2004/10/21 16:45:51  d3x0r
// Updaes to dialog handling... still ahve  aproblem with caption resize
//
// Revision 1.17  2004/10/09 01:35:28  d3x0r
// Updates for latest api stuff...
//
// Revision 1.16  2004/06/01 21:46:09  d3x0r
// Oops I mean really fix it this time.
//
// Revision 1.15  2004/06/01 21:35:24  d3x0r
// Make build linux friendly
//
// Revision 1.14  2004/05/27 09:17:52  d3x0r
// Add font alpha depth to font choice dialog
//
// Revision 1.13  2004/05/27 08:54:43  d3x0r
// Okay fonttst seems to work better - had some kind of infinite load recusion...
//
// Revision 1.12  2003/10/07 20:29:49  panther
// Modify render to accept flags, test program to test bits.  Generate multi-bit alpha
//
// Revision 1.11  2003/10/07 02:12:50  panther
// Ug - it's all terribly broken
//
// Revision 1.10  2003/10/07 02:01:09  panther
// 8 bit alpha fonts....
//
// Revision 1.9  2003/10/07 00:37:34  panther
// Prior commit in error - Begin render fonts in multi-alpha.
//
// Revision 1.8  2003/10/07 00:32:08  panther
// Fix default font.  Add bit size flag to font
//
// Revision 1.7  2003/09/26 16:44:48  panther
// Extend font test program
//
// Revision 1.6  2003/09/26 11:21:10  panther
// Remove logging, generalize testing...
//
// Revision 1.5  2003/08/27 07:58:39  panther
// Lots of fixes from testing null pointers in listbox, font generation exception protection
//
// Revision 1.4  2003/08/21 13:34:42  panther
// include font render project with windows since there's now freetype
//
// Revision 1.3  2003/03/25 08:45:57  panther
// Added CVS logging tag
//
