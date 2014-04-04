
#define MAKE_RCOORD_SINGLE
#include <stdhdrs.h>
#include <vectlib.h>
#define SALTY_RANDOM_GENERATOR_SOURCE
#include <salty_generator.h>

struct grid
{
	int x, y, x2, y2;
	int skip_left;
	int skip_top;
};

struct plasma_state
{
	RCOORD corners[4];
	int seed_corner;
	struct random_context *entropy;
	RCOORD *map;
	size_t stride;
};

void PlasmaFill( struct plasma_state *plasma, struct grid *here )
{
	int mx, my;
	struct grid next;
	RCOORD del;
	RCOORD *map = plasma->map;
	mx = ( here->x2 + here->x ) / 2;
	my = ( here->y2 + here->y ) / 2;

	if( mx != here->x )
	{
		RCOORD del1 = ( ( SRG_GetEntropy( plasma->entropy, 5, FALSE ) / 32.0 ) - 0.5 );
		RCOORD del2 = ( ( SRG_GetEntropy( plasma->entropy, 5, FALSE ) / 32.0 ) - 0.5 );
		RCOORD area = ( mx - here->x ) / (RCOORD)500;
		if( !here->skip_top )
		{
			map[mx + here->y * plasma->stride] = ( map[here->x + here->y*plasma->stride] + map[here->x2 + here->y*plasma->stride] ) / 2
				+ area * del1;
			if( map[mx + here->y * plasma->stride] > 1.0 )
				map[mx + here->y * plasma->stride] = 1.0;
			if( map[mx + here->y * plasma->stride] < 0.0 )
				map[mx + here->y * plasma->stride] = 0.0;
		}

		map[mx + here->y2 * plasma->stride] = ( map[here->x + here->y2*plasma->stride] + map[here->x2 + here->y2*plasma->stride] ) / 2
			+ area * del2;
		if( map[mx + here->y2 * plasma->stride] > 1.0 )
			map[mx + here->y2 * plasma->stride] = 1.0;
		if( map[mx + here->y2 * plasma->stride] < 0.0 )
			map[mx + here->y2 * plasma->stride] = 0.0;
	}
	if( my != here->y )
	{
		RCOORD del1 = ( ( SRG_GetEntropy( plasma->entropy, 5, FALSE ) / 32.0 ) - 0.5 );
		RCOORD del2 = ( ( SRG_GetEntropy( plasma->entropy, 5, FALSE ) / 32.0 ) - 0.5 );
		RCOORD area = ( my - here->y ) / (RCOORD)500;
		if( !here->skip_left )
		{
			map[here->x + my * plasma->stride] = ( map[here->x + here->y*plasma->stride] + map[here->x + here->y2*plasma->stride] ) / 2
				+ area * del1;
			if( map[here->x + my * plasma->stride] > 1.0 )
				map[here->x + my * plasma->stride] = 1.0;
			if( map[here->x + my * plasma->stride] < 0.0 )
				map[here->x + my * plasma->stride] = 0.0;
		}
		map[here->x2 + my * plasma->stride] = ( map[here->x2 + here->y*plasma->stride] + map[here->x2 + here->y2*plasma->stride] ) / 2
			+ area * del2;
		if( map[here->x2 + my * plasma->stride] > 1.0 )
			map[here->x2 + my * plasma->stride] = 1.0;
		if( map[here->x2 + my * plasma->stride] < 0.0 )
			map[here->x2 + my * plasma->stride] = 0.0;
	}
	if( ( mx != here->x ) && ( my != here->y ) )
	{
		RCOORD del1 = ( ( SRG_GetEntropy( plasma->entropy, 5, FALSE ) / 32.0 ) - 0.5 );
		RCOORD area = ( sqrt( ( ( mx - here->x )*( mx - here->x ) + ( my - here->y )*( my - here->y )) ) / ( 500 ) );
		RCOORD avg = ( map[here->x + here->y*plasma->stride] + map[here->x2 + here->y*plasma->stride]
						 + map[here->x + here->y2*plasma->stride] + map[here->x2 + here->y2*plasma->stride] ) / 4;
		//lprintf( "Set point %d,%d = %g (%g) %g", del1, mx, my, map[mx + my * plasma->stride], avg );
		map[mx + my * plasma->stride] = avg + ( area *  del1 );
		if( map[mx + my * plasma->stride] > 1.0 )
			map[mx + my * plasma->stride] = 1.0;
		if( map[mx + my * plasma->stride] < 0.0 )
			map[mx + my * plasma->stride] = 0.0;
	}

	// x to mx and mx to x2 need to be done...
	if( ( mx != here->x ) && ( my != here->y ) )
	{
		next.x = here->x;
		next.y = here->y;
		next.x2 = mx;
		next.y2 = my;
		next.skip_left = here->skip_left;
		next.skip_top = here->skip_top;
		PlasmaFill( plasma, &next );
		next.x = mx;
		next.y = here->y;
		next.x2 = here->x2;
		next.y2 = my;
		next.skip_left = 1;
		next.skip_top = here->skip_top;
		PlasmaFill( plasma, &next );
		next.x = here->x;
		next.y = my;
		next.x2 = mx;
		next.y2 = here->y2;
		next.skip_left = here->skip_left;
		next.skip_top = 1;
		PlasmaFill( plasma, &next );
		next.x = mx;
		next.y = my;
		next.x2 = here->x2;
		next.y2 = here->y2;
		next.skip_left = 1;
		next.skip_top = 1;
		PlasmaFill( plasma, &next );
	}
	else if( mx != here->x )
	{
		next.x = here->x;
		next.y = here->y;
		next.x2 = mx;
		next.y2 = here->y2;
		next.skip_left = here->skip_left;
		next.skip_top = here->skip_top;
		PlasmaFill( plasma, &next );
		next.x = mx;
		next.y = here->y;
		next.x2 = here->x2;
		next.y2 = here->y2;
		next.skip_top = here->skip_top;
		next.skip_left = 1;
		PlasmaFill( plasma, &next );
	}
	else if( my != here->y )
	{
		next.x = here->x;
		next.y = here->y;
		next.x2 = here->x2;
		next.y2 = my;
		next.skip_left = here->skip_left;
		next.skip_top = here->skip_top;
		PlasmaFill( plasma, &next );
		next.x = here->x;
		next.y = my;
		next.x2 = here->x2;
		next.y2 = here->y2;
		next.skip_top = 1;
		next.skip_left = here->skip_left;
		PlasmaFill( plasma, &next );
	}
}

static void FeedRandom( PTRSZVAL psvPlasma, POINTER *salt, size_t *salt_size )
{
	struct plasma_state *plasma = (struct plasma_state *)psvPlasma;
	(*salt) = &plasma->corners[ plasma->seed_corner ];
	(*salt_size) = sizeof( RCOORD );
	plasma->seed_corner++;
	if( plasma->seed_corner > 3 )
		plasma->seed_corner = 0;
}

struct plasma_state *PlasmaCreate( RCOORD seed[4], int width, int height )
{
	struct plasma_state *plasma = New( struct plasma_state );
	struct grid next;
	plasma->map =  NewArray( RCOORD, width * height );
	plasma->stride = width;
	plasma->seed_corner = 0;
	MemSet( plasma->map, (PTRSZVAL)4.0f, sizeof( RCOORD ) * width * height );
	plasma->map[0 + 0 * width]                    = plasma->corners[0] = seed[0];
	plasma->map[(width - 1) + 0 * width]          = plasma->corners[1] = seed[1];
	plasma->map[0 + (height-1) * width]           = plasma->corners[2] = seed[2];
	plasma->map[(width - 1) + (height-1) * width] = plasma->corners[3] = seed[3];
	plasma->entropy = SRG_CreateEntropy( FeedRandom, (PTRSZVAL)plasma );
	next.x = 0;
	next.y = 0;
	next.x2 = width - 1;
	next.y2 = height - 1;
	next.skip_top = 0;
	next.skip_left = 0;
	PlasmaFill( plasma, &next );
	return plasma;
}

RCOORD *PlasmaGetSurface( struct plasma_state *plasma )
{
	return plasma->map;
}

