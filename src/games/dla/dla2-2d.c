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

#define WORKTYPE _16
struct roamer {
   int x, y;
   struct roamer *next;
   struct roamer **me;
};

struct seeded_node {
	int x, y;
	int branches;
	struct seeded_node *parent;
	struct seeded_node *child;
	struct seeded_node *elder;
	struct seeded_node *younger;
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
	CRITICALSECTION cs_roamers;
	struct roamer *roamer_list;
	struct roamer *roamer_free_list;
	int source_bits;
	int source_count; // number of sources available.
	PLIST sources; // list of source points available.
	PTHREAD updater;
	struct random_context *entropy;
	int generation;

	struct plasma_patch *plasma;
	float delta_table[65][65];
	PLIST root_seeds;
} l;

void InitDeltaTable( void )
{
	int x, y;
	for( x = -32; x <= 32; x++ )
		for( y = -32; y <= 32; y++ )
			l.delta_table[x+32][y+32] 
				= sqrt( ((float)x)/32.0 * ((float)x)/32.0 + ((float)y)/32.0 * ((float)y)/32.0 );
}

void CheckLists( void )
{
	PLIST used = NULL;
	struct roamer *roamer;
	SetLink( &used, 100, 0 );
	for( roamer = l.roamer_free_list; roamer; roamer = roamer->next )
	{
		if( FindLink( &used, roamer ) != INVALID_INDEX )
			DebugBreak();
		AddLink( &used, roamer );
	}
	DeleteList( &used );
	SetLink( &used, 100, 0 );
	for( roamer = l.roamer_list; roamer; roamer = roamer->next )
	{
		if( FindLink( &used, roamer ) != INVALID_INDEX )
			DebugBreak();
		AddLink( &used, roamer );
	}
}

void FreeRoamer( struct roamer *roamer )
{
	if( (*roamer->me) = roamer->next )
		roamer->next->me = roamer->me;
	if( roamer->next = l.roamer_free_list )
		l.roamer_free_list->me = &roamer->next;
	roamer->me = &l.roamer_free_list;
	l.roamer_free_list = roamer;
	l.roamer_count--;
}

struct roamer *GetRoamer( void )
{
	struct roamer *roamer;
	if( roamer = l.roamer_free_list )
	{
		if( (*roamer->me) = roamer->next )
			roamer->next->me = roamer->me;
		if( roamer->next = l.roamer_list )
			l.roamer_list->me = &roamer->next;
		roamer->me = &l.roamer_list;
		l.roamer_list = roamer;
	}
	l.roamer_count++;
	return roamer;
}

LOGICAL FindAttachFrom( struct seeded_node *node, struct roamer *roamer, int generation )
{
	if( node )
	if( roamer->x >= ( node->x - 32 ) && roamer->x <= ( node->x + 32 ) )
		if( roamer->y >= ( node->y - 32 ) && roamer->y <= ( node->y + 32 ) )
		{
			if( l.delta_table[roamer->x - node->x + 32][roamer->y - node->y + 32] * ( 0.7 * node->branches + 0.5* ((float)generation+7)/8 ) 
				<= ( 1.0f ) )
			{
				struct seeded_node *next = New( struct seeded_node );
				next->branches = 1;
				next->x = roamer->x;
				next->y = roamer->y;
				next->parent = node;
				if( next->elder = node->child )
					node->child->younger = next;
				next->child = NULL;
				next->younger = NULL;
				node->child = next;
				node->branches++;
				FreeRoamer( roamer );
				return TRUE;
			}
		}
	while( node )
	{
		if( FindAttachFrom( node->child, roamer, generation + 1 ) )
			return TRUE;
		node = node->elder;
	}
	return FALSE;
}

LOGICAL FindAttach( struct roamer *roamer )
{
	INDEX idx;
	struct seeded_node *node;
	//for( node = l.root_seeds
	LIST_FORALL( l.root_seeds, idx, struct seeded_node *, node )
	{
		if( FindAttachFrom( node, roamer, 1 ) )
			return TRUE;
	}
	return FALSE;
}

void DrawChains( Image output, struct seeded_node *node, int generation )
{
	struct seeded_node *child;
	for( child = node->child; child; child = child->elder )
	{
		do_line( output, ( node->x - SURFACE_PAD ) /*/ 2 + SURFACE_SIZE/4*/, ( node->y - SURFACE_PAD ) /*/ 2 + SURFACE_SIZE/4*/
				, ( child->x - SURFACE_PAD ) /*/ 2 + SURFACE_SIZE/4*/, ( child->y - SURFACE_PAD ) /*/ 2 + SURFACE_SIZE/4*/
				, ColorAverage( l.range_colors[0], l.range_colors[1], generation, 64 ) );
		DrawChains( output, child, generation+1 );
	}
}

void OnDrawDisplay( PTRSZVAL psvUser, PRENDERER r )
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
	{
		struct seeded_node *root;
		LIST_FORALL( l.root_seeds, x, struct seeded_node *, root )
			DrawChains( output, root, 0 );
	}
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
	{
		INDEX idx;
		struct roamer *roamer;
		EnterCriticalSec( &l.cs_roamers );
		for( roamer = l.roamer_list; roamer; roamer = roamer->next )
		{
			if( ( roamer->x > SURFACE_PAD ) 
				&& ( roamer->y > SURFACE_PAD )
				&& roamer->x < (SURFACE_PAD+SURFACE_SIZE)
				&& roamer->y < (SURFACE_PAD+SURFACE_SIZE) )
				image[ ( roamer->x - SURFACE_PAD ) + ( ( (output->height - ( roamer->y - SURFACE_PAD ) ) ) * SURFACE_SIZE )] 
						= l.white;
		}
		LeaveCriticalSec( &l.cs_roamers );
	}
#endif
	UpdateDisplay( r );
	//WakeThread( l.updater );
}

void CPROC TickUpdateDisplay( PTRSZVAL psv )
{
   Redraw( l.r );
}

void AddRoamer( void )
{
	int index = SRG_GetEntropy( l.entropy, l.source_bits, 0 );
	struct roamer *roamer = GetRoamer(  );
	struct roamer *source;
	int x, y;
	WORKTYPE *here;
	if( roamer )
	{
		index %= l.source_count;
		source = (struct roamer*)GetLink( &l.sources, index );
#if 1
		x = source->x;
		y = source->y;
#else
		do
		{
			x = SURFACE_PAD + SRG_GetEntropy( l.entropy, 20, 0 ) % SURFACE_SIZE;
			y = SURFACE_PAD + SRG_GetEntropy( l.entropy, 20, 0 ) % SURFACE_SIZE;
			here = l.working_plane + ( x ) + ( y ) * WORK_SIZE;
		}
		while( here[0] || here[-1] || here[1] || here[WORK_SIZE] || here[-WORK_SIZE]
				|| here[-1+WORK_SIZE] || here[-1-WORK_SIZE]
				|| here[1+WORK_SIZE] || here[1-WORK_SIZE] );
#endif
		roamer->x = x;
		roamer->y = y;
	}
}

void UpdateRoamers( int start )
{
	INDEX idx;
	struct roamer *roamer;
	struct roamer *next;
	int d;
	int generated = 0;
	int generation = ( l.generation / 30 ) + 1;
	idx = start * 1000;
	next = NULL;
	for( roamer = l.roamer_list; roamer; roamer = next )
	{
		EnterCriticalSec( &l.cs_roamers );
		next = roamer->next;
#if 0
		roamer->y++;
#endif
		d = SRG_GetEntropy( l.entropy, 2, 0 );
		switch( d )
		{
		case 0:
			if( roamer->x > 0 )
				roamer->x--;
			else
			{
				FreeRoamer( roamer );
				roamer = NULL;
			}
			break;
		case 1:
			if( roamer->y > 0 )
				roamer->y--;
			else
			{
				FreeRoamer( roamer );
	            roamer = NULL;
			}
			break;
		case 2:
			if( roamer->x < (WORK_SIZE-1) )
				roamer->x++;
			else
			{
				FreeRoamer( roamer );
				roamer = NULL;
			}
			break;
		case 3:
			if( roamer->y < (WORK_SIZE-1) )
				roamer->y++;
			else
			{
				FreeRoamer( roamer );
				roamer = NULL;
			}
			break;
		}
		if( roamer )
		{
			if( FindAttach( roamer ) )
				generated = TRUE;
		}
	LeaveCriticalSec( &l.cs_roamers );
	}
	if( generated )
		l.generation++;

}

PTRSZVAL CPROC UpdateRoamerThread( PTHREAD thread )
{
	int start = GetThreadParam( thread );
	l.updater = thread;

		while( l.roamer_count >= start * 100 && l.roamer_count < (start+1) * 100 )
			AddRoamer();
	while ( 1 )
	{
		if( l.roamer_count >= start * 1000 && l.roamer_count < (start+1) * 1000 )
			AddRoamer();
		UpdateRoamers( start );
		//Redraw( l.r );
		//WakeableSleep( 1000 );
		Relinquish();
	}
}

void InitRoamers( void )
{
	int n;
	struct roamer *roamer;
	for( n = 0; n < 10000; n++ )
	{
		roamer = New( struct roamer );
		if( roamer->next = l.roamer_free_list )
			l.roamer_free_list->me = &roamer->next;
		roamer->me = &l.roamer_free_list;
		l.roamer_free_list = roamer;
	}
}

void InitSkyAndLand( void )
{
	int n;
	for( n = 0; n < SURFACE_SIZE/ 32; n++ )
	{
		struct seeded_node * root;
		root = New( struct seeded_node );
		root->branches = 0;
		root->x = n * 32 + SURFACE_PAD;
		root->y = SURFACE_SIZE + SURFACE_PAD;
		root->parent = NULL;
		root->child = NULL;
		root->elder = NULL;
		root->younger = NULL;
		AddLink( &l.root_seeds, root );
		
	}

	for( n = 0; n < SURFACE_SIZE; n += 16 )
	{
		struct roamer *source = New( struct roamer );
		source->x = SURFACE_PAD + n;
		source->y = SURFACE_PAD;
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
	struct seeded_node *root;
#define CENTER_SIZE 10

	root = New( struct seeded_node );
	root->branches = 0;
	root->x = SURFACE_SIZE/2 + SURFACE_PAD;
	root->y = SURFACE_SIZE/2 + SURFACE_PAD;
	root->parent = NULL;
	root->child = NULL;
	root->elder = NULL;
	root->younger = NULL;
	AddLink( &l.root_seeds, root );

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

 void MoreSalt( PTRSZVAL psv, POINTER *salt, size_t *salt_size )
 {
	 static _32 val;
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
	InitializeCriticalSec( &l.cs_roamers );
	l.entropy = SRG_CreateEntropy2( MoreSalt, 0 );
	InitDeltaTable();
	InitRoamers();
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
