
#define MAKE_RCOORD_SINGLE
#include <stdhdrs.h>
#include <vectlib.h>
#define SALTY_RANDOM_GENERATOR_SOURCE
#include <salty_generator.h>

struct grid
{
	int x, y, x2, y2;
	struct 
	{
		BIT_FIELD skip_left : 1;
		BIT_FIELD skip_top : 1;
		BIT_FIELD skip_right : 1;
		BIT_FIELD skip_bottom : 1;
	};
};

struct plasma_state
{
	RCOORD corners[4];
	int seed_corner;
	int saved_seed_corner;
	RCOORD min_height;
	RCOORD max_height;
	RCOORD _min_height;
	RCOORD _max_height;
	struct random_context *entropy;
	RCOORD *map;
	size_t stride, rows;
	RCOORD area_scalar;
	RCOORD horiz_area_scalar;
	POINTER entopy_state;

};

void PlasmaFill2( struct plasma_state *plasma, RCOORD *map, struct grid *here )
{
	static PDATAQUEUE pdq_todo;
	int mx, my;
	struct grid real_here;
	struct grid next;
	RCOORD del;
	RCOORD center;
	RCOORD this_point;
	MemCpy( &real_here, here, sizeof( struct grid ) );
	here = &real_here;

#define mid(a,b) (((a)+(b))/2)

	if( !pdq_todo )
		pdq_todo = CreateLargeDataQueueEx( sizeof( struct grid ), plasma->rows*plasma->stride/4, plasma->rows*plasma->stride/64 DBG_SRC );

	do
	{
		mx = ( here->x2 + here->x ) / 2;
		my = ( here->y2 + here->y ) / 2;

		// may be a pinched rectangle... 3x2 that just has 2 mids top bottom to fill no center
		//lprintf( "center %d,%d  next is %d,%d %d,%d", mx, my, here->x, here->y, here->x2, here->y2 );

		if( ( mx != here->x ) && ( my != here->y ) )
		{
			RCOORD del1 = ( ( SRG_GetEntropy( plasma->entropy, 7, FALSE ) / 128.0 ) - 0.5 );
			RCOORD area = ( sqrt( ( ( here->x2 - here->x )*( here->x2 - here->x ) + ( here->y2 - here->y )*( here->y2 - here->y )) ) / ( plasma->area_scalar ) );
			RCOORD avg = ( map[here->x + here->y*plasma->stride] + map[here->x2 + here->y*plasma->stride]
							 + map[here->x + here->y2*plasma->stride] + map[here->x2 + here->y2*plasma->stride] ) / 4;
			//avg += ( map[here->x + my*plasma->stride] + map[here->x2 + my*plasma->stride]
			//				 + map[mx + here->y*plasma->stride] + map[mx + here->y2*plasma->stride] ) / 4;
			//avg /= 2;
			//lprintf( "Set point %d,%d = %g (%g) %g", mx, my, map[mx + my * plasma->stride], avg );
			center 
				= this_point
				= map[mx + my * plasma->stride] = avg + ( area *  del1 );
			/*
			if( map[mx + my * plasma->stride] > 1.0 )
				map[mx + my * plasma->stride] = 1.0;
			if( map[mx + my * plasma->stride] < 0.0 )
				map[mx + my * plasma->stride] = 0.0;
			*/
				if( this_point > plasma->max_height )
					plasma->max_height = this_point;
				if( this_point < plasma->min_height )
					plasma->min_height = this_point;

			if( mid( next.x = here->x, next.x2 = mx ) != next.x 
				&& mid( next.y = here->y, next.y2 = my ) != next.y  )
			{
				//next.x = here->x;
				//next.y = here->y;
				//next.x2 = mx;
				//next.y2 = my;
				next.skip_left = here->skip_left;
				next.skip_top = here->skip_top;
				EnqueData( &pdq_todo, &next );
			}
			if( mid( next.x = mx, next.x2 = here->x2 ) != next.x 
				&& mid( next.y = here->y, next.y2 = my ) != next.y  )
			{
				//next.x = mx;
				//next.y = here->y;
				//next.x2 = here->x2;
				//next.y2 = my;
				next.skip_left = 1;
				next.skip_top = here->skip_top;
				EnqueData( &pdq_todo, &next );
			}
			if( mid( next.x = here->x, next.x2 = mx ) != next.x 
				&& mid( next.y = my, next.y2 = here->y2 ) != next.y  )
			{
				//next.x = here->x;
				//next.y = my;
				//next.x2 = mx;
				//next.y2 = here->y2;
				next.skip_left = here->skip_left;
				next.skip_top = 1;
				EnqueData( &pdq_todo, &next );
			}
			if( mid( next.x = mx, next.x2 = here->x2 ) != next.x 
				&& mid( next.y = my, next.y2 = here->y2 ) != next.y  )
			{
				//next.x = mx;
				//next.y = my;
				//next.x2 = here->x2;
				//next.y2 = here->y2;
				next.skip_left = 1;
				next.skip_top = 1;
				EnqueData( &pdq_todo, &next );
			}
		}
		else 
		{
			lprintf( "Squre, never happens..." );
			if( mx != here->x )
			{
				center = ( map[here->x + here->y*plasma->stride] + map[here->x2 + here->y*plasma->stride] ) / 2;
				if( mid( next.x = here->x, next.x2 = mx ) != next.x 
					&& mid( next.y = here->y, next.y2 = here->y2 ) != next.y  )
				{
					//next.x = here->x;
					//next.y = here->y;
					//next.x2 = mx;
					//next.y2 = here->y2;
					next.skip_left = here->skip_left;
					next.skip_top = here->skip_top;
					EnqueData( &pdq_todo, &next );
				}
				if( mid( next.x = mx, next.x2 = here->x2 ) != next.x 
					&& mid( next.y = here->y, next.y2 = here->y2 ) != next.y  )
				{
					//next.x = mx;
					//next.y = here->y;
					//next.x2 = here->x2;
					//next.y2 = here->y2;
					next.skip_top = here->skip_top;
					next.skip_left = 1;
					EnqueData( &pdq_todo, &next );
				}
			}
			else
			{
				center = ( map[here->x + here->y*plasma->stride] + map[here->x + here->y2*plasma->stride] ) / 2;
				if( mid( next.x = here->x, next.x2 = here->x2 ) != next.x 
					&& mid( next.y = here->y, next.y2 = my ) != next.y  )
				{
					//next.x = here->x;
					//next.y = here->y;
					//next.x2 = here->x2;
					//next.y2 = my;
					next.skip_left = here->skip_left;
					next.skip_top = here->skip_top;
					EnqueData( &pdq_todo, &next );
				}
				if( mid( next.x = here->x, next.x2 = here->x2 ) != next.x 
					&& mid( next.y = my, next.y2 = here->y2 ) != next.y  )
				{
					//next.x = here->x;
					//next.y = my;
					//next.x2 = here->x2;
					//next.y2 = here->y2;
					next.skip_top = 1;
					next.skip_left = here->skip_left;
					EnqueData( &pdq_todo, &next );
				}
			}
		}
		if( mx != here->x )
		{
			RCOORD del1 = ( ( SRG_GetEntropy( plasma->entropy, 7, FALSE ) / 128.0 ) - 0.5 );
			RCOORD del2 = ( ( SRG_GetEntropy( plasma->entropy, 7, FALSE ) / 128.0 ) - 0.5 );
			RCOORD area = ( mx - here->x ) / ( plasma->horiz_area_scalar );
			if( !here->skip_top )
			{
				//lprintf( "set point  %d,%d", mx, here->y );
				this_point
					= map[mx + here->y * plasma->stride] 
					= ( map[here->x + here->y*plasma->stride] + map[here->x2 + here->y*plasma->stride] + center ) / 3
						+ area * del1;
				/*
				if( map[mx + here->y * plasma->stride] > 1.0 )
					map[mx + here->y * plasma->stride] = 1.0;
				if( map[mx + here->y * plasma->stride] < 0.0 )
					map[mx + here->y * plasma->stride] = 0.0;
					*/
				if( this_point > plasma->max_height )
					plasma->max_height = this_point;
				if( this_point < plasma->min_height )
					plasma->min_height = this_point;
			}

			//lprintf( "set point  %d,%d", mx, here->y2 );
			this_point
					= map[mx + here->y2 * plasma->stride] 
				= ( map[here->x + here->y2*plasma->stride] + map[here->x2 + here->y2*plasma->stride] + center ) / 3
				+ area * del2;
			/*
			if( map[mx + here->y2 * plasma->stride] > 1.0 )
				map[mx + here->y2 * plasma->stride] = 1.0;
			if( map[mx + here->y2 * plasma->stride] < 0.0 )
				map[mx + here->y2 * plasma->stride] = 0.0;
				*/
				if( this_point > plasma->max_height )
					plasma->max_height = this_point;
				if( this_point < plasma->min_height )
					plasma->min_height = this_point;
		}
		if( my != here->y )
		{
			RCOORD del1 = ( ( SRG_GetEntropy( plasma->entropy, 7, FALSE ) / 128.0 ) - 0.5 );
			RCOORD del2 = ( ( SRG_GetEntropy( plasma->entropy, 7, FALSE ) / 128.0 ) - 0.5 );
			RCOORD area = ( my - here->y ) / ( plasma->horiz_area_scalar );
			if( !here->skip_left )
			{
				this_point
					= map[here->x + my * plasma->stride] = ( map[here->x + here->y*plasma->stride] + map[here->x + here->y2*plasma->stride] + center ) / 3
					+ area * del1;
				//lprintf( "set point  %d,%d", here->x, my );
				/*
				if( map[here->x + my * plasma->stride] > 1.0 )
					map[here->x + my * plasma->stride] = 1.0;
				if( map[here->x + my * plasma->stride] < 0.0 )
					map[here->x + my * plasma->stride] = 0.0;
					*/
				if( this_point > plasma->max_height )
					plasma->max_height = this_point;
				if( this_point < plasma->min_height )
					plasma->min_height = this_point;
			}
			this_point
					= map[here->x2 + my * plasma->stride] = ( map[here->x2 + here->y*plasma->stride] + map[here->x2 + here->y2*plasma->stride] + center ) / 3
				+ area * del2;
			//lprintf( "set point  %d,%d", here->x2, my );
			/*
			if( map[here->x2 + my * plasma->stride] > 1.0 )
				map[here->x2 + my * plasma->stride] = 1.0;
			if( map[here->x2 + my * plasma->stride] < 0.0 )
				map[here->x2 + my * plasma->stride] = 0.0;
				*/
				if( this_point > plasma->max_height )
					plasma->max_height = this_point;
				if( this_point < plasma->min_height )
					plasma->min_height = this_point;

		}
		else
			lprintf( "can't happen" );

	}
	while( DequeData( &pdq_todo, &real_here ) );
	//DeleteDataQueue( &pdq_todo );
	// x to mx and mx to x2 need to be done...
}

void PlasmaFill( struct plasma_state *plasma, RCOORD *map, struct grid *here )
{
	int mx, my;
	struct grid next;
	RCOORD del;
	RCOORD center;
	RCOORD this_point;
	mx = ( here->x2 + here->x ) / 2;
	my = ( here->y2 + here->y ) / 2;

	if( ( mx != here->x ) && ( my != here->y ) )
	{
		RCOORD del1 = ( ( SRG_GetEntropy( plasma->entropy, 7, FALSE ) / 128.0 ) - 0.5 );
		RCOORD area = ( sqrt( ( ( here->x2 - here->x )*( here->x2 - here->x ) + ( here->y2 - here->y )*( here->y2 - here->y )) ) / ( plasma->area_scalar ) );
		RCOORD avg = ( map[here->x + here->y*plasma->stride] + map[here->x2 + here->y*plasma->stride]
						 + map[here->x + here->y2*plasma->stride] + map[here->x2 + here->y2*plasma->stride] ) / 4;
		//avg += ( map[here->x + my*plasma->stride] + map[here->x2 + my*plasma->stride]
		//				 + map[mx + here->y*plasma->stride] + map[mx + here->y2*plasma->stride] ) / 4;
		//avg /= 2;
		//lprintf( "Set point %d,%d = %g (%g) %g", del1, mx, my, map[mx + my * plasma->stride], avg );
		center 
			= this_point
			= map[mx + my * plasma->stride] = avg + ( area *  del1 );
		/*
		if( map[mx + my * plasma->stride] > 1.0 )
			map[mx + my * plasma->stride] = 1.0;
		if( map[mx + my * plasma->stride] < 0.0 )
			map[mx + my * plasma->stride] = 0.0;
		*/
			if( this_point > plasma->max_height )
				plasma->max_height = this_point;
			if( this_point < plasma->min_height )
				plasma->min_height = this_point;
	}
	else 
		if( mx != here->x )
			center = ( map[here->x + here->y*plasma->stride] + map[here->x2 + here->y*plasma->stride] ) / 2;
		else
			center = ( map[here->x + here->y*plasma->stride] + map[here->x + here->y2*plasma->stride] ) / 2;

	if( mx != here->x )
	{
		RCOORD del1 = ( ( SRG_GetEntropy( plasma->entropy, 7, FALSE ) / 128.0 ) - 0.5 );
		RCOORD del2 = ( ( SRG_GetEntropy( plasma->entropy, 7, FALSE ) / 128.0 ) - 0.5 );
		RCOORD area = ( mx - here->x ) / ( plasma->horiz_area_scalar );
		if( !here->skip_top )
		{
			this_point
				= map[mx + here->y * plasma->stride] 
				= ( map[here->x + here->y*plasma->stride] + map[here->x2 + here->y*plasma->stride] + center ) / 3
					+ area * del1;
			/*
			if( map[mx + here->y * plasma->stride] > 1.0 )
				map[mx + here->y * plasma->stride] = 1.0;
			if( map[mx + here->y * plasma->stride] < 0.0 )
				map[mx + here->y * plasma->stride] = 0.0;
				*/
			if( this_point > plasma->max_height )
				plasma->max_height = this_point;
			if( this_point < plasma->min_height )
				plasma->min_height = this_point;
		}

		this_point
				= map[mx + here->y2 * plasma->stride] 
			= ( map[here->x + here->y2*plasma->stride] + map[here->x2 + here->y2*plasma->stride] + center ) / 3
			+ area * del2;
		/*
		if( map[mx + here->y2 * plasma->stride] > 1.0 )
			map[mx + here->y2 * plasma->stride] = 1.0;
		if( map[mx + here->y2 * plasma->stride] < 0.0 )
			map[mx + here->y2 * plasma->stride] = 0.0;
			*/
			if( this_point > plasma->max_height )
				plasma->max_height = this_point;
			if( this_point < plasma->min_height )
				plasma->min_height = this_point;
	}
	if( my != here->y )
	{
		RCOORD del1 = ( ( SRG_GetEntropy( plasma->entropy, 7, FALSE ) / 128.0 ) - 0.5 );
		RCOORD del2 = ( ( SRG_GetEntropy( plasma->entropy, 7, FALSE ) / 128.0 ) - 0.5 );
		RCOORD area = ( my - here->y ) / ( plasma->horiz_area_scalar );
		if( !here->skip_left )
		{
			this_point
				= map[here->x + my * plasma->stride] = ( map[here->x + here->y*plasma->stride] + map[here->x + here->y2*plasma->stride] + center ) / 3
				+ area * del1;
			/*
			if( map[here->x + my * plasma->stride] > 1.0 )
				map[here->x + my * plasma->stride] = 1.0;
			if( map[here->x + my * plasma->stride] < 0.0 )
				map[here->x + my * plasma->stride] = 0.0;
				*/
			if( this_point > plasma->max_height )
				plasma->max_height = this_point;
			if( this_point < plasma->min_height )
				plasma->min_height = this_point;
		}
		this_point
				= map[here->x2 + my * plasma->stride] = ( map[here->x2 + here->y*plasma->stride] + map[here->x2 + here->y2*plasma->stride] + center ) / 3
			+ area * del2;
		/*
		if( map[here->x2 + my * plasma->stride] > 1.0 )
			map[here->x2 + my * plasma->stride] = 1.0;
		if( map[here->x2 + my * plasma->stride] < 0.0 )
			map[here->x2 + my * plasma->stride] = 0.0;
			*/
			if( this_point > plasma->max_height )
				plasma->max_height = this_point;
			if( this_point < plasma->min_height )
				plasma->min_height = this_point;
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
		PlasmaFill( plasma, map, &next );
		next.x = mx;
		next.y = here->y;
		next.x2 = here->x2;
		next.y2 = my;
		next.skip_left = 1;
		next.skip_top = here->skip_top;
		PlasmaFill( plasma, map, &next );
		next.x = here->x;
		next.y = my;
		next.x2 = mx;
		next.y2 = here->y2;
		next.skip_left = here->skip_left;
		next.skip_top = 1;
		PlasmaFill( plasma, map, &next );
		next.x = mx;
		next.y = my;
		next.x2 = here->x2;
		next.y2 = here->y2;
		next.skip_left = 1;
		next.skip_top = 1;
		PlasmaFill( plasma, map, &next );
	}
	else if( mx != here->x )
	{
		next.x = here->x;
		next.y = here->y;
		next.x2 = mx;
		next.y2 = here->y2;
		next.skip_left = here->skip_left;
		next.skip_top = here->skip_top;
		PlasmaFill( plasma, map, &next );
		next.x = mx;
		next.y = here->y;
		next.x2 = here->x2;
		next.y2 = here->y2;
		next.skip_top = here->skip_top;
		next.skip_left = 1;
		PlasmaFill( plasma, map, &next );
	}
	else if( my != here->y )
	{
		next.x = here->x;
		next.y = here->y;
		next.x2 = here->x2;
		next.y2 = my;
		next.skip_left = here->skip_left;
		next.skip_top = here->skip_top;
		PlasmaFill( plasma, map, &next );
		next.x = here->x;
		next.y = my;
		next.x2 = here->x2;
		next.y2 = here->y2;
		next.skip_top = 1;
		next.skip_left = here->skip_left;
		PlasmaFill( plasma, map, &next );
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

void PlasmaRender( struct plasma_state *plasma, RCOORD *seed )
{
	struct grid next;
	RCOORD *map1 =  NewArray( RCOORD, plasma->stride * plasma->rows );
	//RCOORD *map2 =  NewArray( RCOORD, plasma->stride * plasma->rows );
	if( plasma->entopy_state )
	{
		SRG_RestoreState( plasma->entropy, plasma->entopy_state );
		plasma->seed_corner = plasma->saved_seed_corner;
	
	}
	else
	{
		SRG_SaveState( plasma->entropy, &plasma->entopy_state );
		plasma->saved_seed_corner = plasma->seed_corner;
	}
	plasma->min_height = 0;
	plasma->max_height = 0;
	next.x = 0;
	next.y = 0;
	next.x2 = plasma->stride - 1;
	next.y2 = plasma->rows - 1;
	next.skip_top = 0;
	next.skip_left = 0;

	if( !seed )
		seed = plasma->corners;
	map1[0 + 0 * plasma->stride]                    = plasma->corners[0] = seed[0];
	map1[(plasma->stride - 1) + 0 * plasma->stride]          = plasma->corners[1] = seed[1];
	map1[0 + (plasma->rows-1) * plasma->stride]           = plasma->corners[2] = seed[2];
	map1[(plasma->stride - 1) + (plasma->rows-1) * plasma->stride] = plasma->corners[3] = seed[3];

	{
		int n;
		plasma->_min_height = 0;
		plasma->_max_height = 1;
		plasma->min_height = 1;
		plasma->max_height = 0;
		/*
		for( n = 0; n < 4; n++ )
		{
			RCOORD this_point = plasma->corners[n];
			if( this_point > plasma->max_height )
				plasma->_max_height
					= plasma->max_height 
					= this_point;
			if( this_point < plasma->min_height )
				plasma->_min_height 
					= plasma->min_height
					= this_point;
		}
		*/
	}

	//PlasmaFill( plasma, map1, &next );
	PlasmaFill2( plasma, map1, &next );

	//map2[0 + 0 * plasma->stride]                    = plasma->corners[0];
	//map2[(plasma->stride - 1) + 0 * plasma->stride]          = plasma->corners[1];
	//map2[0 + (plasma->rows-1) * plasma->stride]           = plasma->corners[2];
	//map2[(plasma->stride - 1) + (plasma->rows-1) * plasma->stride] = plasma->corners[3];
	//PlasmaFill( plasma, map2, &next );

	{
		int x, y;
		RCOORD *map = plasma->map;
		RCOORD *map_from = map1;
		lprintf( "map result min and max: %g %g ", plasma->min_height, plasma->max_height );
		if( ( plasma->min_height < plasma->_min_height || plasma->max_height > plasma->_max_height ) )
		{
		for( x = 0; x < plasma->stride; x++ )
			for( y = 0; y < plasma->rows; y++ )
			{
				RCOORD input = map_from[0];
				LOGICAL updated;
				do
				{
					updated  = FALSE;
					if( input > plasma->_max_height )
					{
						// 
						input = plasma->_max_height - ( input - plasma->_max_height );
						updated = TRUE;
					}
					else if( input < plasma->_min_height )
					{
						input = plasma->_min_height - ( input - plasma->_min_height );
						updated = TRUE;
					}
				}
				while( updated );
				if( 1 ) // mode == 1 )
				{
				// need to specify a copy mode.
					map[0] = input;
					//sin( /*map2[index] * */( map_from[0] - plasma->min_height ) / divider * 3.14159/2 /*+ map2[index] * 3.14159/2*/ );
					//( 1 + sin( /*map2[index] * */( map_from[0] - plasma->min_height ) / divider * 3.14159 - 3.14159/2 /*+ map2[index] * 3.14159/2*/ ) ) / 2;
				}

				else if( 1 ) // mode == 2 )
				{
					// smooth top and bottom across sin curve, middle span is 1:1 ...
					map[0] = 
						( 1 + sin( input * 3.14159 - 3.14159/2 ) ) / 2;
				}

				if( 0 ) // mode == 3 )  // bad mode ... needs work
				{
				// peaker tops and bottoms smoother middle, middle span ...
				map[0] = 
					( 1 + tan( ( ( input ) + 0.5 ) * ( 3.14159 * 0.5 ) + (3.14159/2) ) ) /2;
				}

				if( 0 ) // mode == 4 ) // use square furnction, parabolic... cubic... qudric?
				// peaker tops and bottoms smoother middle, middle span is 1:1 ...
				{
					RCOORD tmp = input - 0.5;
					if( tmp < 0 )
						map[0] = 
							( 0.5 + ( 2 * tmp * tmp ) );
					else
						map[0] = 
							0.5 -( 2 * tmp * tmp );
				}
				//*/
				map++;
				map_from++;
				//lprintf( "%g = %g %g", plasma->map[index], map1[index], map2[index] );
			}
		}
		else
		{
		RCOORD divider = ( plasma->max_height - plasma->min_height );
		for( x = 0; x < plasma->stride; x++ )
			for( y = 0; y < plasma->rows; y++ )
			{
				// need to specify a copy mode.
				map[0] = 
					//map_from[0];
					( map_from[0] );
					//sin( /*map2[index] * */( map_from[0] - plasma->min_height ) / divider * 3.14159/2 /*+ map2[index] * 3.14159/2*/ );
					//( 1 + sin( /*map2[index] * */( map_from[0] - plasma->min_height ) / divider * 3.14159 - 3.14159/2 /*+ map2[index] * 3.14159/2*/ ) ) / 2;

				/*
				// smooth top and bottom across sin curve, middle span is 1:1 ...
				map[0] = 
					( 1 + sin( ( map_from[0] - plasma->min_height ) / divider * 3.14159 - 3.14159/2 ) ) / 2;
				*/
				///*
				// peaker tops and bottoms smoother middle, middle span ...
				//map[0] = 
				//	( 1 + tan( ( ( ( map_from[0] - plasma->min_height ) / divider ) + 0.5 ) * ( 3.14159 * 0.5 ) + (3.14159/2) ) ) /2;
				// peaker tops and bottoms smoother middle, middle span is 1:1 ...
				if( 0 )
				{
					RCOORD tmp = ( map_from[0] - plasma->min_height ) / divider - 0.5;
					if( tmp < 0 )
						map[0] = 
							( 0.5 + ( 2 * tmp * tmp ) );
					else
						map[0] = 
							0.5 -( 2 * tmp * tmp );
				}
				//*/
				map++;
				map_from++;
				//lprintf( "%g = %g %g", plasma->map[index], map1[index], map2[index] );
			}
		}
	}
	Release( map1 );
	//Release( map2 );
}

struct plasma_state *PlasmaCreate( RCOORD seed[4], RCOORD roughness, int width, int height )
{
	struct plasma_state *plasma = New( struct plasma_state );
	struct grid next;
	plasma->map =  NewArray( RCOORD, width * height );
	plasma->stride = width;
	plasma->rows = height;
	plasma->seed_corner = 0;
	plasma->area_scalar = roughness;
	plasma->horiz_area_scalar = roughness;
	plasma->entopy_state = NULL;
	plasma->corners[0] = seed[0];
	plasma->corners[1] = seed[1];
	plasma->corners[2] = seed[2];
	plasma->corners[3] = seed[3];
	{
		int n;
		plasma->_min_height = 0;
		plasma->_max_height = 1;
		plasma->min_height = 1;
		plasma->max_height = 0;
		/*
		for( n = 0; n < 4; n++ )
		{
			RCOORD this_point = plasma->corners[n];
			if( this_point > plasma->max_height )
				plasma->_max_height
					= plasma->max_height 
					= this_point;
			if( this_point < plasma->min_height )
				plasma->_min_height 
					= plasma->min_height
					= this_point;
		}
		*/
	}

	plasma->entropy = SRG_CreateEntropy( FeedRandom, (PTRSZVAL)plasma );

	PlasmaRender( plasma, plasma->corners );

	return plasma;
}

RCOORD *PlasmaGetSurface( struct plasma_state *plasma )
{
	return plasma->map;
}

void PlasmaSetRoughness( struct plasma_state *plasma, RCOORD roughness, RCOORD horiz_rough )
{
	plasma->area_scalar = roughness;
	plasma->horiz_area_scalar = roughness * horiz_rough;
}


