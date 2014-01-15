
/**********************************************************************

Auxiliary program to generate font definitions in .h type format
using "symbits.h" as defining values to display


***********************************************************************/

//#define LOG_INITIAL
//#define LOG_CLIPPED
//#define LOG_FINAL

//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

FILE *output;

int PrintChar( char *charid, char *data, int szx, int szy
					, int baseline
					, int cellx
					, int xorg, int yorg
					, int width, int height 
					, int asc, int desc )
{
	int b, x, y, newline = 0;
	int yofs, outwidth;

	fprintf( output, WIDE("// Character: size x,y ( %d, %d ) baseline = %d \n")
				        "// origin x,y ( %d, %d ) width,height ( %d, %d )\n"
			         , szx, szy 
			         , baseline
						, xorg, yorg
						, width, height );
	

	#define LINEPAD "                  "


	outwidth = ((width+7) & 0xF8); // round up to next byte increments size.

	if( ((outwidth)/8)*szy )
		fprintf( output, WIDE("static struct{ char s, w, o, j, a, d; unsigned char data[%d]; } %s =\n"), 
						((outwidth)/8)*szy, charid );    1
	else
		fprintf( output, WIDE("static struct{ char s, w, o, j, a, d; } %s =\n")
						, charid );

	fprintf( output, WIDE("{ %2d, %2d, %2d, 0, %2d, %2d"), width, szx, xorg, asc, desc);

	if( outwidth )
		fprintf( output, WIDE(", {") );

	yofs = baseline-yorg;

	for( y = 0; y < yofs; y++ )
	{
		b = 0;
		if( outwidth )
		{
			if( y == baseline )
				fprintf( output, WIDE(", // <---- baseline\n") LINEPAD );
			else
				if( newline )
					fprintf( output, WIDE(",\n") LINEPAD );
		}
		for( x = 0; x < outwidth; x++ )
		{
			fprintf( output, WIDE("_") );
			if( b < (outwidth-1) )
   			if( ( b & 7 ) == 7 )
	   			fprintf( output, WIDE(",") );
			b++;
		}
		newline=1;
	}
	if( yofs < 0 )
		yofs = 0;

	for( y = 0; y < height; y++ )
	{
		b = 0;
		if( (yofs  + y) == baseline )
			fprintf( output, WIDE(", // <---- baseline\n") LINEPAD );
		else
			if( newline )
				fprintf( output, WIDE(",\n") LINEPAD );

		for( x = 0; x < width; x++ )
		{
			if( (*(data + (x/8)) ) & ( 0x80U >> (x & 0x7) ) )
				fprintf( output, WIDE("X") );
			else
				fprintf( output, WIDE("_") );
			if( b < (width-1) )
   			if( ( b & 7 ) == 7 )
	   			fprintf( output, WIDE(",") );
			b++;
		}

		for( x = width; x < outwidth; x++ )
		{
			fprintf( output, WIDE("_") );
			if( b < (outwidth-1) )
   			if( ( b & 7 ) == 7 )
	   			fprintf( output, WIDE(",") );
			b++;
		}
		data += ((outwidth+31)/32)*4;
		newline=1;
	}
	for( y = height + yofs; y < szy ; y++ )
	{
		b = 0;
		if( outwidth )
		{
			if( (y) == baseline )
				fprintf( output, WIDE(", // <---- baseline\n") LINEPAD );
			else
				if( newline )
					fprintf( output, WIDE(",\n") LINEPAD );
		}
		for( x = 0; x < outwidth; x++ )
		{
			fprintf( output, WIDE("_") );
			if( b < (outwidth-1) )
				if( ( b & 7 ) == 7 )
					fprintf( output, WIDE(",") );
			b++;
		}
		newline=1;
	}
	if( outwidth )
		fprintf( output, WIDE("} ") );
   fprintf( output, WIDE("};\n") );
	return 0;
}




int FixData( int character
				, char *data_in, char *out_in
				, int baseline
				, GLYPHMETRICS *gm
				, int bywidth, int width, int height )
{
	char *data, *out;
	int x, y, min = width, max = 0, n, mindata, maxdata, maxline;
	data = data_in;
	data += (bywidth)* (height-1);
	out = out_in;
#if defined( LOGINITIAL ) || defined( LOG_CLIPPED ) || defined( LOG_FINAL )
   printf( WIDE("--------------- Character compuation --------------------\n") );
#endif
	for( y = 0; y < height; y++ )
	{
		for( n = 0; n < bywidth; n++ )
		{
			out[n] = data[n];
		}
		for( x = 0; x < width; x++ )
		{
			if( out[x/8] & ( 0x80>>(x&7) ) )
			{
				if( x < min )
					min = x;
				if( x > max )
					max = x;
				#ifdef LOG_INITIAL
					printf( WIDE("X") );
				#endif
			}
			else
			{
				#ifdef LOG_INITIAL
					printf( WIDE("_") );
				#endif
			}
		}
		#ifdef LOG_INITIAL
			printf( WIDE("\n") );
		#endif

		out += bywidth;
		data -= bywidth;
	}
	if( min == 0 )
	{
		fprintf( output, WIDE("\nFont may be corrupt left overflow\n") );
		printf( WIDE("\nFont may be corrupt left overflow\n") );
	}
	if( max == width-1 )
	{
		fprintf( output, WIDE("\nFont may be corrupt right overflow\n") );
		printf( WIDE("\nFont may be corrupt right overflow\n") );
	}
	if( width - max < 5 )
	{
      fprintf( stderr, WIDE("Character is wider than max metric %d\n"), width-max );
	}
	if( min < 5 )
	{
      fprintf( stderr, WIDE("Character begins before baseline %d %d\n"), min, character );
	}
	out = out_in;
	data = data_in;
	mindata = width;
	maxdata = 0;
	maxline = 0;
	gm->gmptGlyphOrigin.x = min;
	gm->gmptGlyphOrigin.y = height;
	for( y = 0; y < height; y++ )
	{
		int setbit;
		for( n = 0; n < (bywidth+3) & 0xFFFFFC; n++ )
		{
			data[n] = 0;
		}
		setbit = 0;
		for( x = min; x <= max; x++ )
		{
			if( out[x/8] & ( 0x80 >> (x&7) ) )
			{
				#ifdef LOG_CLIPPED
				   printf( WIDE("X") );
				#endif
			}
			else
			{
				if( y < gm->gmptGlyphOrigin.y )
					gm->gmptGlyphOrigin.y = y;
				if( y > maxline )
					maxline = y;
				if( x < mindata )
					mindata = x;
				if( x > maxdata )
					maxdata = x;
				#ifdef LOG_CLIPPED
					printf( WIDE("_") );
				#endif
			}
		}
		out += bywidth;
		#ifdef LOG_CLIPPED
	 		printf( WIDE("\n") );
		#endif
	}	
	gm->gmptGlyphOrigin.x = mindata - 5;
   gm->gmBlackBoxX = (maxdata-mindata)+1;
	out = out_in + (gm->gmptGlyphOrigin.y * bywidth);
	data = data_in;

	if( maxline == 0 )
	{
      gm->gmBlackBoxX = 0;
      gm->gmptGlyphOrigin.x = 0;
      gm->gmBlackBoxY = 0;
      gm->gmptGlyphOrigin.y = 0;
	}
	else
	{
		// need to add 8 here - +1 cause max/min are INCLUSIVE
      int outstep = ( ( ( maxdata - mindata + 8 ) / 8 ) + 3 ) & 0xFFFFFC;
		#ifdef LOG_FINAL
	      printf( WIDE("Max/min = %d/%d step=%d\n"), maxdata, mindata, outstep );
		#endif
 	 	for( y = gm->gmptGlyphOrigin.y; y <= maxline; y++ )
 	 	{
			int setbit;
			for( n = 0; n < outstep; n++ )
			{
				data[n] = 0;
			}
			setbit = 0;
 	 		for( x = mindata; x <= maxdata; x++ )
 	 		{
				if( out[x/8] & ( 0x80 >> (x&7) ) )
				{
					#ifdef LOG_FINAL
						printf( WIDE("X") );
					#endif							
				}
				else
				{
					data[setbit/8] |= (0x80 >> (setbit&7));
					#ifdef LOG_FINAL
						printf( WIDE("_") );
					#endif
				}
 	 			setbit++;
 	 		}
			out += bywidth;
			data += outstep;
			#ifdef LOG_FINAL
	 	 		printf( WIDE("\n") );
			#endif
 	 	}
 	   
      gm->gmBlackBoxY = (maxline-gm->gmptGlyphOrigin.y)+1;
      gm->gmptGlyphOrigin.y = baseline - gm->gmptGlyphOrigin.y;
	}
	return min;
}

void KillSpaces( char *string )
{
	int ofs = 0;
	if( !string )
		return;
	while( *string )
	{
		string[ofs] = string[0];
		if( *string == ' ' )
			ofs--;
		string++;
	}
	string[ofs] = 0;
}



#define FONT 0

int main( void )
{
	LOGFONT logfont;
	HFONT   hfont;
	HDC	  hdc;
	int     loop = 0;
	char    fontname[256];
#if (FONT==0)
   loop = 1;
#elif (FONT==1)
	 // bold title font 1
	logfont.lfHeight = 20;
	logfont.lfWidth = 8;
	logfont.lfEscapement = 0;
	logfont.lfOrientation = 0;
	logfont.lfWeight = FW_BOLD;
   logfont.lfCharSet = 0;
	logfont.lfStrikeOut = 0;
	logfont.lfUnderline = 0;
	logfont.lfItalic = 0;
	logfont.lfOutPrecision = 0; //OUT_CHARACTER_PRECIS;
	logfont.lfClipPrecision = 0;
	logfont.lfQuality = 0;
	logfont.lfPitchAndFamily = FIXED_PITCH;
	strcpy(logfont.lfFaceName, WIDE("Courier New"));
#elif (FONT==2)	

  	logfont.lfHeight = 16;
	logfont.lfWidth = 4;
	logfont.lfEscapement = 0;
	logfont.lfOrientation = 0;
	logfont.lfWeight = FW_NORMAL;
    logfont.lfCharSet = 0;
	logfont.lfStrikeOut = 0;
	logfont.lfUnderline = 0;
	logfont.lfItalic = 0;
	logfont.lfOutPrecision = 0;
	logfont.lfClipPrecision = 0;
	logfont.lfQuality = 0;
	logfont.lfPitchAndFamily = 0;
	strcpy(logfont.lfFaceName, WIDE("MS Serif"));
#elif (FONT==3)
	 // bold title font 1
	logfont.lfHeight = 80;
	logfont.lfWidth = 50;
	logfont.lfEscapement = 0;
	logfont.lfOrientation = 0;
	logfont.lfWeight = FW_BOLD;
   logfont.lfCharSet = 0;
	logfont.lfStrikeOut = 0;
	logfont.lfUnderline = 0;
	logfont.lfItalic = 0;
	logfont.lfOutPrecision = 0; //OUT_CHARACTER_PRECIS;
	logfont.lfClipPrecision = 0;
	logfont.lfQuality = 0;
	logfont.lfPitchAndFamily = FIXED_PITCH;
	strcpy(logfont.lfFaceName, WIDE("Courier New"));
#endif
	//if(0)
	while( 1 )
	{
		if(loop)
		{
			CHOOSEFONT cf;
			memset( &cf, 0, sizeof( CHOOSEFONT ) );
			cf.lStructSize = sizeof( CHOOSEFONT );
			cf.lpLogFont = &logfont;
			cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS ;
			if( !ChooseFont( &cf ) )
			{
				printf( WIDE("okay all done, thanx for dumping...\n") );
				return 0;
			}
		}
		loop = 1;
		hfont = CreateFontIndirect(&logfont);

		hdc = CreateDC( WIDE("DISPLAY"), NULL, NULL, NULL );

		if( !hdc )
		{
			printf( WIDE("Sorry no DC :(\n") );
			exit(0);
		}
		SelectObject( hdc, hfont );


		{
			HDC 		hdcmem;
			HBITMAP 	hbitmapmem, holdbitmap;

			MAT2         mat;

			BITMAPINFO  *bmInfo;
			char *chardata, *buffer, *fixedbuffer;

			//OUTLINETEXTMETRIC *otm;
			TEXTMETRIC tm;
			int otmsize;
			int i;
			RECT rcmem;
			HBRUSH hBrush0;
			mat.eM11.value = 1;
			mat.eM12.value = 0;
			mat.eM21.value = 0;
			mat.eM22.value = 1;
			mat.eM11.fract = 0;
			mat.eM12.fract = 0;
			mat.eM21.fract = 0;
			mat.eM22.fract = 0;
			if( !GetTextMetrics( hdc, &tm ) )
			{
				printf( WIDE("Sorry we don't like something... bad dc, bad font? %d\n"), GetLastError() );
				return 0;
			}
			{
				char data[256];
				char attribs[6];
				int idx;
				strcpy( data, logfont.lfFaceName );
				KillSpaces( data );
				idx = 0;
				if( logfont.lfWeight == FW_BOLD )
					attribs[idx++] = 'B';
				if( logfont.lfItalic )
					attribs[idx++] = 'I';
				attribs[idx] = 0;
				sprintf( fontname, WIDE("%s%dby%d%s")
							, data
							, (logfont.lfHeight<0)?(-logfont.lfHeight):(logfont.lfHeight)
							, (logfont.lfWidth)?logfont.lfWidth:tm.tmMaxCharWidth
							, attribs
							, tm.tmHeight );
			}
			{
				char filename[256];
				sprintf( filename, WIDE("%s.c"), fontname );
				output = fopen( filename, WIDE("wt") );
				if( !output )
				{
					printf( WIDE("Failed to open file to write: %s\n"), filename );
					exit(0);
				}
				printf( WIDE("Writing font: %s\n"), fontname );
			}

			hdcmem = CreateCompatibleDC( hdc );
			SelectObject( hdcmem, hfont );
			hbitmapmem = CreateCompatibleBitmap( hdcmem, tm.tmMaxCharWidth + 10, tm.tmHeight );
			holdbitmap = SelectObject( hdcmem, hbitmapmem );


			bmInfo = malloc( sizeof( BITMAPINFO ) + sizeof( RGBQUAD ) );
			memset( bmInfo, 0, sizeof( BITMAPINFO ) + sizeof( RGBQUAD ) );
			bmInfo->bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
			bmInfo->bmiHeader.biBitCount = 0;

			holdbitmap = SelectObject( hdcmem, holdbitmap );
			if( !GetDIBits( hdcmem, hbitmapmem, 0, 0, NULL, bmInfo, DIB_PAL_COLORS ) )
			{
				printf( WIDE("GetDIBits failed: %d\n"), GetLastError() );
			}
			holdbitmap = SelectObject( hdcmem, holdbitmap );

			if( bmInfo->bmiHeader.biBitCount != 1 )
			{
				printf( WIDE("We'll have problems if our bitmap is not 1 bpp %d\n")
						  , bmInfo->bmiHeader.biBitCount );
				return 0;
			}
			rcmem.left = 0;
			rcmem.top = 0;
			rcmem.right = bmInfo->bmiHeader.biWidth;
			rcmem.bottom = bmInfo->bmiHeader.biHeight;
			printf( WIDE("Bitmap is: %d by %d\n"), bmInfo->bmiHeader.biWidth
					  , bmInfo->bmiHeader.biHeight );
			//bmInfo->bmiHeader.biHeight *= -1;
			{
				LOGBRUSH lb;
				printf( WIDE("Brush is %08lx\n"), *(long*)&bmInfo->bmiColors[0] );
				lb.lbStyle = BS_SOLID;
				lb.lbColor = *(long*)&bmInfo->bmiColors[0];
				lb.lbHatch = 0;
				hBrush0 = CreateBrushIndirect( &lb );
			}
			chardata = (char*)malloc( bmInfo->bmiHeader.biHeight *
											  (( ( ( bmInfo->bmiHeader.biWidth + 7 ) / 8 ) + 3 ) & 0xFFFFFC) );
			fixedbuffer = (char*)malloc( bmInfo->bmiHeader.biHeight *
												  (( ( ( bmInfo->bmiHeader.biWidth + 7 ) / 8 ) + 3 ) & 0xFFFFFC) );
			//		fixedbuffer = (char*)	malloc( bmInfo->bmiHeader.biSizeImage );
			//otmsize = GetOutlineTextMetrics( hdc, 0, NULL );
			//if( !otmsize )
			//{
			//	printf( WIDE("Sorry we don't like something... bad dc, bad font? %d\n"), GetLastError() );
			//	return 0;
			//}
			//otm = (OUTLINETEXTMETRIC*)malloc( otmsize );
			//otm->otmSize = sizeof( OUTLINETEXTMETRIC );
			//GetOutlineTextMetrics( hdc, otmsize, otm );

			fprintf( output, WIDE("#include \")symbits.h\"\n" );
			fprintf( output, WIDE("typedef char CHARACTER, *PCHARACTER;\n") );

			printf( WIDE("textcolor is: %08lx\n"), *(long*)&bmInfo->bmiColors[1] );
			SetTextColor( hdcmem, *(long*)&bmInfo->bmiColors[1] );
			for( i = 0; i < 256; i++ )
			{
				GLYPHMETRICS gm;
				char tmp = i;
				SIZE sz;
				char charid[25];
				int bufsize;
				sprintf( charid, WIDE("_char_%d"), i );
				GetTextExtentPoint( hdc, (char*)&i, 1, &sz );
				//bufsize = GetGlyphOutline( hdc, i, GGO_BITMAP, &gm, 0, NULL, &mat );
				bufsize = -1;
				gm.gmCellIncX = sz.cx;
				// cellincy is useless...
				if( bufsize > 0 )
				{
					buffer = (char*)malloc( bufsize );
					GetGlyphOutline( hdc, i, GGO_BITMAP, &gm, bufsize, buffer, &mat );
					PrintChar( charid, (char*)buffer, sz.cx
								  , tm.tmHeight
								  , tm.tmAscent
								  , gm.gmCellIncX
								  , gm.gmptGlyphOrigin.x
								  , gm.gmptGlyphOrigin.y
								  , gm.gmBlackBoxX, gm.gmBlackBoxY );
					free( buffer );
				}
				else
				{
					//printf( WIDE("GetGlyphOutline failed: %d\n"), GetLastError() );
					FillRect( hdcmem, &rcmem, hBrush0 );
					if( !TextOut( hdcmem, 5, 0, (char*)&i, 1 ) )
					{
						printf( WIDE("TextOut Failed: %d\n"), GetLastError() );
						continue;
					}
					holdbitmap = SelectObject( hdcmem, holdbitmap );

					if( GetDIBits( hdcmem, hbitmapmem, 0
										, bmInfo->bmiHeader.biHeight
										, chardata, bmInfo, DIB_PAL_COLORS ) != bmInfo->bmiHeader.biHeight )
					{
						printf( WIDE("Got short bits?\n") );
					}
					FixData( i, chardata, fixedbuffer, tm.tmAscent, &gm
								, bmInfo->bmiHeader.biSizeImage / bmInfo->bmiHeader.biHeight
								, bmInfo->bmiHeader.biWidth
								, bmInfo->bmiHeader.biHeight );

					PrintChar( charid, (char*)chardata, sz.cx
								  , tm.tmHeight
								  , tm.tmAscent
								  , gm.gmCellIncX
								  , gm.gmptGlyphOrigin.x
								  , gm.gmptGlyphOrigin.y
								  , gm.gmBlackBoxX, gm.gmBlackBoxY );

					fprintf( output, WIDE("\n\n"));
					holdbitmap = SelectObject( hdcmem, holdbitmap );
				}
			}

			{
				char data[256];
				char attribs[6];
				int idx;
				strcpy( data, logfont.lfFaceName );
				KillSpaces( data );
				idx = 0;
				if( logfont.lfWeight == FW_BOLD )
					attribs[idx++] = 'B';
				if( logfont.lfItalic )
					attribs[idx++] = 'I';
				attribs[idx] = 0;
				fprintf( output, WIDE("struct { unsigned short height, chars;\n")
							"         PCHARACTER character[256]; } %s%dby%d%s = { %d, 256, {"
							, data
							, (logfont.lfHeight<0)?(-logfont.lfHeight):(logfont.lfHeight)
							, (logfont.lfWidth)?logfont.lfWidth:tm.tmMaxCharWidth
							, attribs
							, tm.tmHeight );

				for( i = 0; i < 256; i++ )
				{
					fprintf( output, WIDE(" %c(PCHARACTER)&_char_%d\n"), (i)?',':' ', i );

				}
				fprintf( output, WIDE("\n} };") );
			}
			//free( otm );
			fclose( output );
		}
	}
	return 0;
}

// $Log: createfont.c,v $
// Revision 1.7  2003/03/25 09:37:58  panther
// Fix file tails mangled by CVS logging
//
// Revision 1.6  2003/03/25 08:45:52  panther
// Added CVS logging tag
//
