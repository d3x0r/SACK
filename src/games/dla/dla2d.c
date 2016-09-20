#include <stdhdrs.h>
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER_INTERFACE l.pdi
#include <image.h>
#include <render.h>
#include <salty_generator.h>
#include "../plasma_grid/plasma.h"

#define SURFACE_SIZE 1024
#define SURFACE_PAD 100
#define WORK_SIZE ((2*SURFACE_PAD)+SURFACE_SIZE)

#define WORKTYPE uint16_t
struct roamer {
   int x, y;
};

struct dla_local
{
	PIMAGE_INTERFACE pii;
	PRENDER_INTERFACE pdi;
	PRENDERER r;
	CDATA range_colors[5];
	CDATA white;
	WORKTYPE *working_plane;
	int roamer_count;
	PLIST roamers;
	int source_bits;
	int source_count; // number of sources available.
	PLIST sources; // list of source points available.
	PTHREAD updater;
	struct random_context *entropy;
	int generation;

	struct plasma_patch *plasma;

} l;




void OnDrawDisplay( uintptr_t psvUser, PRENDERER r )
{
	int x, y;
	Image output = GetDisplayImage( r );
	CDATA *image = (CDATA*)output->image;
	RCOORD *data 
		= //use_grid_reader?GridReader_Read( l.grid_reader, 1000 + l.ofs_x, 1000 + l.ofs_y, patch, patch )
			//: 
			PlasmaReadSurface( l.plasma, 0, 0, 0 );
#undef plot
#define plot(a,b,c,d) do{ (*_output) = d; _output++; } while(0)
	ClearImage( output );
#if 0
	// draw plasma state
	{
		int w, h;
		CDATA *_output;
		for( h = 0; h < output->height; h++ )
		{
			_output = image + ( output->height - h - 1 ) * output->pwidth;

			for( w = 0; w < output->width; w++ )
			{
				RCOORD here = data[ h * output->width + w ];
				if( here <= 0.01 )
					plot( output, w, h, ColorAverage( BASE_COLOR_WHITE,
													 BASE_COLOR_BLACK, here * 1000, 250 ) );
				else if( here <= 0.25 )
					plot( output, w, h, ColorAverage( BASE_COLOR_BLACK,
													 BASE_COLOR_LIGHTBLUE, (here) * 1000, 250 ) );
				else if( here <= 0.5 )
					plot( output, w, h, ColorAverage( BASE_COLOR_LIGHTBLUE,
													 BASE_COLOR_LIGHTGREEN, (here-0.25) * 1000, 250 ) );
				else if( here <= 0.75 )
					plot( output, w, h, ColorAverage( BASE_COLOR_LIGHTGREEN,
													 BASE_COLOR_LIGHTRED, (here-0.5) * 1000, 250 ) );
				else if( here <= 0.99 )
					plot( output, w, h, ColorAverage( BASE_COLOR_LIGHTRED,
													 BASE_COLOR_WHITE, (here-0.75) * 1000, 250 ) );
				else //if( here <= 4.0 / 4 )
					plot( output, w, h, ColorAverage( BASE_COLOR_WHITE,
													 BASE_COLOR_BLACK, (here-0.99) * 10000, 100 ) );
				//lprintf( "%d,%d  %g", w, h, data[ h * output->width + w ] );
			}
		}
	}
#endif
#if 1
	for( x = 0; x < SURFACE_SIZE; x++ )
		for( y = 0; y < SURFACE_SIZE; y++ )
		{
			WORKTYPE b;
			if( b = l.working_plane[ (y + SURFACE_PAD) * WORK_SIZE + x + SURFACE_PAD] )
				image[ ( (y) ) * SURFACE_SIZE+ ( x ) ] 
					= ColorAverage( l.range_colors[(b/64)%5], l.range_colors[((b/64) + 1) % 5], b%64, 64 );
		}
	{
		INDEX idx;
		struct roamer *roamer;
		LIST_FORALL( l.roamers, idx, struct roamer *, roamer )
		{
			if( ( roamer->x > SURFACE_PAD ) 
				&& ( roamer->y > SURFACE_PAD )
				&& roamer->x < (SURFACE_PAD+SURFACE_SIZE)
				&& roamer->y < (SURFACE_PAD+SURFACE_SIZE) )
				image[ ( roamer->x - SURFACE_PAD ) + ( ( (roamer->y - SURFACE_PAD) ) * SURFACE_SIZE )] 
						= l.white;
		}
	}
#endif
	UpdateDisplay( r );
	//WakeThread( l.updater );
}

void CPROC TickUpdateDisplay( uintptr_t psv )
{
   Redraw( l.r );
}

void AddRoamer( void )
{
	int index = SRG_GetEntropy( l.entropy, l.source_bits, 0 );
	struct roamer *roamer = New( struct roamer );
	struct roamer *source;
	int x, y;
	WORKTYPE *here;
	index %= l.source_count;
	source = (struct roamer*)GetLink( &l.sources, index );
	do
	{
		x = SURFACE_PAD + SRG_GetEntropy( l.entropy, 20, 0 ) % SURFACE_SIZE;
		y = SURFACE_PAD + SRG_GetEntropy( l.entropy, 20, 0 ) % SURFACE_SIZE;
		here = l.working_plane + ( x ) + ( y ) * WORK_SIZE;
	}
	while( here[0] || here[-1] || here[1] || here[WORK_SIZE] || here[-WORK_SIZE]
			|| here[-1+WORK_SIZE] || here[-1-WORK_SIZE]
			|| here[1+WORK_SIZE] || here[1-WORK_SIZE] );
	roamer->x = x;
	roamer->y = y;
	AddLink( &l.roamers, roamer );
	l.roamer_count++;
}

void UpdateRoamers( int start )
{
	INDEX idx;
	struct roamer *roamer;
	int d;
	int generated = 0;
	int generation = ( l.generation / 30 ) + 1;
	idx = start * 1000;
	LIST_NEXTALL( l.roamers, idx, struct roamer *, roamer )
	{
		d = SRG_GetEntropy( l.entropy, 2, 0 );
		switch( d )
		{
		case 0:
			if( roamer->x > 0 )
				roamer->x--;
			else
			{
				SetLink( &l.roamers, idx, 0 );
				Release( roamer );
				roamer = NULL;
				l.roamer_count--;
			}
			break;
		case 1:
			if( roamer->y > 0 )
				roamer->y--;
			else
			{
				SetLink( &l.roamers, idx, 0 );
				Release( roamer );
	            roamer = NULL;
				l.roamer_count--;
			}
			break;
		case 2:
			if( roamer->x < (WORK_SIZE-1) )
				roamer->x++;
			else
			{
				SetLink( &l.roamers, idx, 0 );
				Release( roamer );
				roamer = NULL;
				l.roamer_count--;
			}
			break;
		case 3:
			if( roamer->y < (WORK_SIZE-1) )
				roamer->y++;
			else
			{
				SetLink( &l.roamers, idx, 0 );
				Release( roamer );
				roamer = NULL;
				l.roamer_count--;
			}
			break;
		}
		if( roamer )
		{
			WORKTYPE b;
			WORKTYPE *here = l.working_plane + ( roamer->x + roamer->y * WORK_SIZE );
#define NEXTVAL(a) generation
#define NEXTVAL(a) a + 1
			if( !here[0]
				&& ( roamer->x > 0/*SURFACE_PAD*/ ) 
				&& roamer->x < ( SURFACE_SIZE+2*SURFACE_PAD - 1)
				&& ( roamer->y > 0/*SURFACE_PAD*/ ) 
				&& roamer->y < ( SURFACE_SIZE+2*SURFACE_PAD - 1) )
			{
				if( here[-1] )
					here[0] = NEXTVAL(here[-1]);
				else if( here[1] )
					here[0] = NEXTVAL(here[1]);
#if 1
				else if( here[1 + WORK_SIZE] )
					here[0] = NEXTVAL(here[1 + WORK_SIZE]);
				else if( here[1 - WORK_SIZE] )
					here[0] = NEXTVAL(here[1 - WORK_SIZE]);

				else if( here[-1 + WORK_SIZE] )
					here[0] = NEXTVAL(here[-1 + WORK_SIZE]);
				else if( here[-1 - WORK_SIZE] )
					here[0] = NEXTVAL(here[-1 - WORK_SIZE]);
#endif
				else if( here[WORK_SIZE] )
					here[0] = NEXTVAL(here[WORK_SIZE]);
				else if( here[-WORK_SIZE] )
					here[0] = NEXTVAL(here[-WORK_SIZE]);
				if( here[0] )
				{
					SetLink( &l.roamers, idx, 0 );
					Release( roamer );
					l.roamer_count--;
					generated++;
				}
			}
		}
	}
	if( generated )
		l.generation++;

}

uintptr_t CPROC UpdateRoamerThread( PTHREAD thread )
{
	int start = GetThreadParam( thread );
	l.updater = thread;
	SetLink( &l.roamers, 120000, 1 );
	SetLink( &l.roamers, 120000, 0 );
		while( l.roamer_count >= start * 100000 && l.roamer_count < (start+1) * 100000 )
			AddRoamer();
	while ( 1 )
	{
		//if( l.roamer_count >= start * 1000 && l.roamer_count < (start+1) * 1000 )
		//	AddRoamer();
		UpdateRoamers( start );
		//Redraw( l.r );
		//WakeableSleep( 1000 );
		Relinquish();
	}
}

void InitSkyAndLand( void )
{
	int n;
	for( n = 0; n < SURFACE_SIZE; n++ )
	{
		l.working_plane[ (SURFACE_PAD) * ( WORK_SIZE ) 
						+ SURFACE_PAD + n ] = 1;
	}

	for( n = 0; n < SURFACE_SIZE; n += 16 )
	{
		struct roamer *source = New( struct roamer );
		source->x = SURFACE_PAD + n;
		source->y = SURFACE_PAD + SURFACE_SIZE;
		AddLink( &l.sources, source );
		l.source_count++;
	}
	{
		l.source_bits = 0;
		for( n = 0; n < 32; n++ )
			if( l.source_count & ( 1 << n ) )
				l.source_bits = n;
		l.source_bits++;
	}
}

void InitSkyAndSmallLand( void )
{
	int n;
	for( n = 0; n < 20; n++ )
	{
		l.working_plane[ (SURFACE_PAD) * ( WORK_SIZE ) 
						+ SURFACE_PAD + SURFACE_SIZE / 2 - 10 + n ] = 1;
	}
	for( n = 0; n < SURFACE_SIZE; n++ )
	{
		l.working_plane[ (SURFACE_PAD + n) * ( WORK_SIZE ) 
						+ SURFACE_PAD + SURFACE_SIZE / 2  ] = 1;
	}

	for( n = 0; n < SURFACE_SIZE; n += 16 )
	{
		struct roamer *source = New( struct roamer );
		source->x = SURFACE_PAD + n;
		source->y = SURFACE_PAD + SURFACE_SIZE;
		AddLink( &l.sources, source );
		l.source_count++;
	}
	{
		l.source_bits = 0;
		for( n = 0; n < 32; n++ )
			if( l.source_count & ( 1 << n ) )
				l.source_bits = n;
		l.source_bits++;
	}
}

void InitSpaceSparks( void )
{
	int n;
#define CENTER_SIZE 10
	for( n = 0; n < CENTER_SIZE; n++ )
	{
		l.working_plane[ (SURFACE_PAD + (SURFACE_SIZE/2 - (CENTER_SIZE/2)) + n) * ( WORK_SIZE ) 
						+ SURFACE_PAD + (SURFACE_SIZE/2 - (CENTER_SIZE/2))  ] = 1;
		l.working_plane[ (SURFACE_PAD + (SURFACE_SIZE/2 - (CENTER_SIZE/2)) + n) * ( WORK_SIZE ) 
						+ SURFACE_PAD + (SURFACE_SIZE/2 + (CENTER_SIZE/2))  ] = 1;

		l.working_plane[ (SURFACE_PAD + (SURFACE_SIZE/2 - (CENTER_SIZE/2)) ) * ( WORK_SIZE ) 
						+ SURFACE_PAD + (SURFACE_SIZE/2 - (CENTER_SIZE/2) + n)  ] = 1;
		l.working_plane[ (SURFACE_PAD + (SURFACE_SIZE/2 + (CENTER_SIZE/2)) ) * ( WORK_SIZE ) 
						+ SURFACE_PAD + (SURFACE_SIZE/2 - (CENTER_SIZE/2) + n)  ] = 1;
	}

	for( n = 0; n < 125; n ++ )
	{
		struct roamer *source = New( struct roamer );
		int n = SRG_GetEntropy( l.entropy, 20, 0 ) % SURFACE_SIZE * 4;
		if( n < SURFACE_SIZE )
		{
			source->x = n + SURFACE_PAD;
			source->y = SURFACE_PAD;
		}
		else if( n < 2*SURFACE_SIZE )
		{
			source->x = ( n - SURFACE_SIZE ) + SURFACE_PAD;
			source->y = SURFACE_PAD + SURFACE_SIZE;
		}
		else if( n < 3*SURFACE_SIZE )
		{
			source->x = SURFACE_PAD;
			source->y = ( n - SURFACE_SIZE*2) + SURFACE_PAD;
		}
		else if( n < 4*SURFACE_SIZE )
		{
			source->x = SURFACE_SIZE + SURFACE_PAD;
			source->y = ( n - SURFACE_SIZE*3) + SURFACE_PAD;
		}
		AddLink( &l.sources, source );
		l.source_count++;
	}

	{
		l.source_bits = 0;
		for( n = 0; n < 32; n++ )
			if( l.source_count & ( 1 << n ) )
				l.source_bits = n;
		l.source_bits++;
	}
}

 void MoreSalt( uintptr_t psv, POINTER *salt, size_t *salt_size )
 {
	 static uint32_t val;
	 val = GetTickCount();
	 (*salt) = &val;
	 (*salt_size) = sizeof( val );
 }

SaneWinMain( argc, argv )
{
	l.pii = GetImageInterface();
	l.pdi = GetDisplayInterface();
	l.r = OpenDisplaySized( 0, SURFACE_SIZE, SURFACE_SIZE );
	l.working_plane = NewArray( WORKTYPE, ( WORK_SIZE * WORK_SIZE ) );
	MemSet( l.working_plane, 0,  sizeof( WORKTYPE ) * WORK_SIZE * WORK_SIZE );
	l.entropy = SRG_CreateEntropy2( MoreSalt, 0 );

	{
		RCOORD coords[4];
		coords[0] = 0;
		coords[1] = 0;
		coords[2] = 0;
		coords[3] = 0;
		l.plasma = PlasmaCreate( coords, 0.00070/*patch * 2*/, SURFACE_SIZE, SURFACE_SIZE );
	}

	InitSpaceSparks();
	//InitSkyAndLand( );
	//InitSkyAndSmallLand( );

	l.white = BASE_COLOR_WHITE;
	l.range_colors[0] = BASE_COLOR_BLUE;
	l.range_colors[1] = BASE_COLOR_GREEN;
	l.range_colors[2] = BASE_COLOR_RED;
	l.range_colors[3] = BASE_COLOR_MAGENTA;
	l.range_colors[4] = BASE_COLOR_BROWN;

	SetRedrawHandler( l.r, OnDrawDisplay, 0 );
	AddTimer( 250, TickUpdateDisplay, 0 );
	ThreadTo( UpdateRoamerThread, 0 );
	//ThreadTo( UpdateRoamerThread, 1 );
	//ThreadTo( UpdateRoamerThread, 2 );
	//ThreadTo( UpdateRoamerThread, 3 );
	UpdateDisplay( l.r );
	while( 1 )
		WakeableSleep( 10000 );
}
EndSaneWinMain()
