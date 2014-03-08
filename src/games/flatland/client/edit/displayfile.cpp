#include <stdio.h>

#include <logging.h>

#include "flatland_global.h"
#include "display.h"


//----------------------------------------------------------------------------

void ResetDisplay( PDISPLAY display )
{
	SetPoint( display->origin, VectorConst_0 );
	display->flags.bShowSectorText = TRUE;
	display->flags.bShowSectorTexture = TRUE;
	display->flags.bShowLines = FALSE;
	display->flags.bGridTop = FALSE;
	display->zbias = ONE_SCALE; // start at 1:1 perspective

	display->flags.bUseGrid = TRUE;
	display->flags.bShowGrid = TRUE;
	display->GridXUnits = 5;
	display->GridYUnits = 5;
	display->GridXSets = 10;
	display->GridYSets = 10;
	display->GridColor =  Color( 0, 128, 255 );
	display->Background = Color( 0,0,0 );
	display->scale      = 1.0;

	display->NextWall.Wall = INVALID_INDEX;

	ClearUndo(display->pWorld);
}

//----------------------------------------------------------------------------

#define WriteFlag(flagname) {                           \
		char flag;                                        \
		flag = display->flags.flagname;                   \
		sz += fwrite( &flag, 1, sizeof( flag ), file );   \
	}

int WriteDisplayInfo( INDEX iWorld, /*FILE *file, */PDISPLAY display )
{
   FILE *file = sack_fopen( 0, WIDE("display_file_named_by_world"), WIDE("wb") );
   //-------------------------------
   // Version 1: lacked bShowGrid Flag
   // Version 2: lacked display scale
   // Version 3: lacked display on top...
   // Version 4: save origin, current zoom factors, store section size
   // (Current version)
   // Version 5:
   //-------------------------------
	size_t sz = 0;
	int version;
	size_t pos;
	sz += fwrite( WIDE("DISP"), 1, 4, file );
	pos = ftell( file );
	sz += fwrite( &sz, 1, sizeof( sz ), file );
	sz = 0;
	version = 5; // version for display info...
	sz += fwrite( &version, 1, sizeof(version), file );
	WriteFlag( bUseGrid );
	WriteFlag( bShowGrid );
	WriteFlag( bShowSectorText );
	WriteFlag( bShowLines );
	WriteFlag( bShowSectorTexture );
	WriteFlag( bGridTop );
	sz += fwrite( &display->GridXUnits, 1, sizeof( display->GridXUnits), file );
	sz += fwrite( &display->GridYUnits, 1, sizeof( display->GridYUnits), file );
	sz += fwrite( &display->GridXSets, 1,  sizeof( display->GridXSets ), file );
	sz += fwrite( &display->GridYSets, 1,  sizeof( display->GridYSets), file ); 
	sz += fwrite( &display->GridColor, 1,  sizeof( display->GridColor), file );
	sz += fwrite( &display->Background, 1,  sizeof( display->Background), file );
	sz += fwrite( &display->scale, 1, sizeof( display->scale ), file );
	sz += fwrite( &display->origin, 1, sizeof( display->origin ), file );
	sz += fwrite( &display->zbias, 1, sizeof( display->zbias ), file );
   {
		int posend;
		posend = ftell( file );
		fseek( file, pos, SEEK_SET );
		fwrite( &sz, 1, sizeof( sz ), file );
		fseek( file, posend, SEEK_SET );
	}
   fclose( file );
 	return sz;
}

//----------------------------------------------------------------------------

#define ReadFlag(flagname) {                           \
		char flag;                                       \
		sz += fread( &flag, 1, sizeof( flag ), file );   \
		display->flags.flagname = flag;                  \
	}

int ReadDisplayInfo(INDEX iWorld,  PDISPLAY display )
{
	FILE *file = sack_fopen( 0, WIDE("display_file_named_by_world"), WIDE("rb") );
	size_t sz = 0;
	if( file ){
		int version, size;
		TEXTCHAR section[5];
		sz = fread( section, 1, 4, file );
		section[4] = 0;
		if( sz &&
		    !StrCmp( section, WIDE("DISP") ) )
		{
			fread( &size, 1, sizeof( size ), file );
			sz = 0;
			if( size < 20 )
				version = size;
			else
				sz += fread( &version, 1, sizeof( version ), file );
			ReadFlag( bUseGrid );
			if( version > 1 )
			{
				ReadFlag( bShowGrid );
			}
			else
				display->flags.bShowGrid = display->flags.bUseGrid;
			ReadFlag( bShowSectorText );
			ReadFlag( bShowLines );
			ReadFlag( bShowSectorTexture );
			if( version > 3 )
			{
				ReadFlag( bGridTop );
			}
			else
				display->flags.bGridTop = FALSE;
			sz += fread( &display->GridXUnits, 1, sizeof( display->GridXUnits), file );
			sz += fread( &display->GridYUnits, 1, sizeof( display->GridYUnits), file );
			sz += fread( &display->GridXSets, 1,  sizeof( display->GridXSets ), file );
			sz += fread( &display->GridYSets, 1,  sizeof( display->GridYSets), file ); 
			sz += fread( &display->GridColor, 1,  sizeof( display->GridColor), file );
			sz += fread( &display->Background, 1,  sizeof( display->Background), file );
			if( version > 2 )
				sz += fread( &display->scale, 1, sizeof( display->scale ), file );
			else
				display->scale = 1.0;
			if( version > 4 )
			{
				sz += fread( &display->origin, 1, sizeof( display->origin ), file );
				sz += fread( &display->zbias, 1, sizeof( display->zbias ), file );
			}
	        
			if( version > 4 )
			{
				if( sz != size )
					Log2( WIDE("Display section size %d was not %d"), sz, size );
			}
		}
		else
		{
			Log1( WIDE("Display information was not present - setting defaults...(%d)"), sz );
			ResetDisplay( display );
		}
	fclose( file );
}
	return sz;
}

