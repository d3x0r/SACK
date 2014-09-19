
#include <stdhdrs.h>
#include <vectlib.h>
//#define SALTY_RANDOM_GENERATOR_SOURCE
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

struct plasma_patch
{
	// where I am.
	int x, y;
	RCOORD corners[4];
	int seed_corner;
	int saved_seed_corner;
	RCOORD min_height;
	RCOORD max_height;
	RCOORD _min_height;
	RCOORD _max_height;
	struct random_context *entropy;
	RCOORD *map;
	RCOORD *map1;
	RCOORD area_scalar;
	RCOORD horiz_area_scalar;
	POINTER entopy_state;
	struct plasma_state *plasma;


	struct plasma_patch *as_left;
	struct plasma_patch *as_top;
	struct plasma_patch *as_right;
	struct plasma_patch *as_bottom;
};

struct plasma_state
{
	size_t stride, rows;
	RCOORD *read_map;  // this is the map used to retun the current state.
	struct plasma_clip {
		RCOORD top, bottom;
	} clip;
	size_t map_width, map_height;
	int root_x, root_y; // where 0, 0 is...
	struct plasma_patch **world_map;
	RCOORD *world_height_map;
};
RCOORD *PlasmaReadSurface( struct plasma_patch *patch_root, int x, int y, int smoothing, int force_scaling );

void PlasmaFill2( struct plasma_patch *plasma, RCOORD *map, struct grid *here )
{
	static PDATAQUEUE pdq_todo;
	int mx, my;
	struct grid real_here;
	struct grid next;
	RCOORD del;
	RCOORD center;
	RCOORD this_point;
	size_t stride = plasma->plasma->stride;
	size_t rows = plasma->plasma->rows;
	MemCpy( &real_here, here, sizeof( struct grid ) );
	here = &real_here;

#define mid(a,b) (((a)+(b))/2)

	if( !pdq_todo || ( pdq_todo->Cnt < ((rows*stride)/4) ) )
	{
		DeleteDataQueue( &pdq_todo );
		pdq_todo = CreateLargeDataQueueEx( sizeof( struct grid ), rows*stride/4, rows*stride/64 DBG_SRC );
	}

	do
	{
		mx = ( here->x2 + here->x ) / 2;
		my = ( here->y2 + here->y ) / 2;

		// may be a pinched rectangle... 3x2 that just has 2 mids top bottom to fill no center
		//lprintf( "center %d,%d  next is %d,%d %d,%d", mx, my, here->x, here->y, here->x2, here->y2 );

		if( ( mx != here->x ) && ( my != here->y ) )
		{
			RCOORD del1 = ( ( SRG_GetEntropy( plasma->entropy, 13, FALSE ) / (RCOORD)( 1 << 13 ) ) - 0.5 );
			//RCOORD del1 = ( ( SRG_GetEntropy( plasma->entropy, 7, FALSE ) / 128.0 ) - 0.5 );
			RCOORD area = ( sqrt( ( ( here->x2 - here->x )*( here->x2 - here->x ) + ( here->y2 - here->y )*( here->y2 - here->y )) ) * ( plasma->area_scalar ) );
			RCOORD avg = ( map[here->x + here->y*stride] + map[here->x2 + here->y*stride]
							 + map[here->x + here->y2*stride] + map[here->x2 + here->y2*stride] ) / 4;
			//avg += ( map[here->x + my*stride] + map[here->x2 + my*stride]
			//				 + map[mx + here->y*stride] + map[mx + here->y2*stride] ) / 4;
			//avg /= 2;
			//lprintf( "Set point %d,%d = %g (%g) %g", mx, my, map[mx + my * stride], avg );
			center 
				= this_point
				= map[mx + my * stride] = avg + ( area *  del1 );
			/*
			if( map[mx + my * stride] > 1.0 )
				map[mx + my * stride] = 1.0;
			if( map[mx + my * stride] < 0.0 )
				map[mx + my * stride] = 0.0;
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
				center = ( map[here->x + here->y*stride] + map[here->x2 + here->y*stride] ) / 2;
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
				center = ( map[here->x + here->y*stride] + map[here->x + here->y2*stride] ) / 2;
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
			RCOORD del1 = ( ( SRG_GetEntropy( plasma->entropy, 13, FALSE ) / (RCOORD)( 1 << 13 ) ) - 0.5 );
			RCOORD del2 = ( ( SRG_GetEntropy( plasma->entropy, 13, FALSE ) / (RCOORD)( 1 << 13 ) ) - 0.5 );
			RCOORD area = ( mx - here->x ) * ( plasma->horiz_area_scalar );
			if( !here->skip_top && !( here->y == 0 && plasma->as_top ) )
			{
				//lprintf( "set point  %d,%d", mx, here->y );
				this_point
					= map[mx + here->y * stride] 
					= ( map[here->x + here->y*stride] + map[here->x2 + here->y*stride] + center ) / 3
						+ area * del1;
				if( this_point > plasma->max_height )
					plasma->max_height = this_point;
				if( this_point < plasma->min_height )
					plasma->min_height = this_point;
			}

			if( !( here->y2 == (rows-1) && plasma->as_bottom ) )
			{
			//lprintf( "set point  %d,%d", mx, here->y2 );
				this_point
					= map[mx + here->y2 * stride] 
					= ( map[here->x + here->y2*stride] + map[here->x2 + here->y2*stride] + center ) / 3
						+ area * del2;
				if( this_point > plasma->max_height )
					plasma->max_height = this_point;
				if( this_point < plasma->min_height )
					plasma->min_height = this_point;
			}
			//else
				//lprintf( "Skip point %d,%d  %g", here->y2, mx, map[mx + here->y2 * stride] );
		}
		if( my != here->y )
		{
			RCOORD del1 = ( ( SRG_GetEntropy( plasma->entropy, 13, FALSE ) / (RCOORD)( 1 << 13 ) ) - 0.5 );
			RCOORD del2 = ( ( SRG_GetEntropy( plasma->entropy, 13, FALSE ) / (RCOORD)( 1 << 13 ) ) - 0.5 );
			RCOORD area = ( my - here->y ) * ( plasma->horiz_area_scalar );

			if( !here->skip_left && !( here->x == 0 && plasma->as_left ) )
			{
				this_point
					= map[here->x + my * stride] = ( map[here->x + here->y*stride] + map[here->x + here->y2*stride] + center ) / 3
					+ area * del1;
				//lprintf( "set point  %d,%d", here->x, my );
				if( this_point > plasma->max_height )
					plasma->max_height = this_point;
				if( this_point < plasma->min_height )
					plasma->min_height = this_point;
			}
			if( !( here->x2 == (stride-1) && plasma->as_right ) )
			{
				this_point
					= map[here->x2 + my * stride] = ( map[here->x2 + here->y*stride] + map[here->x2 + here->y2*stride] + center ) / 3
						+ area * del2;
				//lprintf( "set point  %d,%d", here->x2, my );
				if( this_point > plasma->max_height )
					plasma->max_height = this_point;
				if( this_point < plasma->min_height )
					plasma->min_height = this_point;
			}
		}
		else
			lprintf( "can't happen" );

	}
	while( DequeData( &pdq_todo, &real_here ) );
	//DeleteDataQueue( &pdq_todo );
	// x to mx and mx to x2 need to be done...
	{
		lprintf( "done..." );
	}
}


static void FeedRandom( PTRSZVAL psvPlasma, POINTER *salt, size_t *salt_size )
{
	struct plasma_patch *plasma = (struct plasma_patch *)psvPlasma;
	if( plasma->seed_corner < 0 )
	{
		(*salt) = &plasma->x;
		(*salt_size) = 2 * sizeof( plasma->x );
	}
	else
	{
		(*salt) = &plasma->corners[ plasma->seed_corner ];
		(*salt_size) = sizeof( RCOORD );
	}
	plasma->seed_corner++;
	if( plasma->seed_corner > 3 )
		plasma->seed_corner = 0;
}

void PlasmaRender( struct plasma_patch *plasma, RCOORD *seed )
{
	struct grid next;
	size_t stride = plasma->plasma->stride;
	size_t rows = plasma->plasma->rows;
	//RCOORD *map2 =  NewArray( RCOORD, stride * plasma->rows );
	if( !seed )
	{
		struct plasma_state *ps = plasma->plasma;
		int x, y;
		for( x = 0; x < ps->map_width; x++ )
			for( y = 0; y < ps->map_height; y++ )
			{
				struct plasma_patch * here;
				if( here = ps->world_map[ ( x ) + ( y ) * ps->map_width] )
					PlasmaRender( here, here->corners );
			}
			return;
	}

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
	next.x2 = stride - 1;
	next.y2 = rows - 1;
	next.skip_top = 0;
	next.skip_left = 0;

	if( !seed )
		seed = plasma->corners;

	if( !plasma->as_left && !plasma->as_top )
		plasma->map1[0 + 0 * stride]                    = plasma->corners[0] = seed[0];

	if( !plasma->as_right && !plasma->as_top )
		plasma->map1[(stride - 1) + 0 * stride]          = plasma->corners[1] = seed[1];

	if( !plasma->as_left && !plasma->as_bottom )
		plasma->map1[0 + (rows-1) * stride]           = plasma->corners[2] = seed[2];

	if( !plasma->as_bottom && !plasma->as_right )
		plasma->map1[(stride - 1) + (rows-1) * stride] = plasma->corners[3] = seed[3];

	{
		int n;
		plasma->_min_height = 1;
		plasma->_max_height = 0;
		plasma->min_height = 1;
		plasma->max_height = 0;
		
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
		
	}

	PlasmaFill2( plasma, plasma->map1, &next );

}

struct plasma_patch *PlasmaCreatePatch( struct plasma_state *map, RCOORD seed[4], RCOORD roughness )
{
	struct plasma_patch *plasma = New( struct plasma_patch );
	struct grid next;
	MemSet( plasma, 0, sizeof( struct plasma_patch ) );
	plasma->plasma = map;

	plasma->map1 =  NewArray( RCOORD, map->stride * map->rows );
	plasma->map =  NewArray( RCOORD, map->stride * map->rows );

	plasma->seed_corner = -1;   // first seed is patch corrdinate, then the corners

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

	plasma->entropy = SRG_CreateEntropy2( FeedRandom, (PTRSZVAL)plasma );


	//if( initial_render )
	//	PlasmaRender( plasma, plasma->corners );

	return plasma;
}

struct plasma_patch *PlasmaCreateEx( RCOORD seed[4], RCOORD roughness, int width, int height, LOGICAL initial_render )
{
	struct plasma_state *plasma = New( struct plasma_state );
	struct plasma_patch *patch;

	plasma->map_height = 32;
	plasma->map_width = 32;

	plasma->root_x = 0;
	plasma->root_y = 0;
	plasma->stride = plasma->map_width;
	plasma->rows = plasma->map_height;
	plasma->read_map =  NewArray( RCOORD, plasma->stride * plasma->rows );
	patch = PlasmaCreatePatch( plasma, seed, 5 );
	PlasmaRender( patch, patch->corners );
	plasma->world_map = &patch;
	plasma->map_width = 1;
	plasma->map_height = 1;
	plasma->clip.top = patch->max_height;
	plasma->clip.bottom = patch->min_height;
	plasma->world_height_map = PlasmaReadSurface( patch, 0, 0, 0, 1 );
	// don't release read_map;
	Release( patch->map1 );
	Release( patch->map );
	Release( patch );

	plasma->map_height = 20;
	plasma->map_width = 20;
	plasma->stride = width;
	plasma->rows = height;
	// make a new read map, the first is actually world_height_map for corner seeds.
	plasma->read_map =  NewArray( RCOORD, plasma->stride * plasma->rows );

	plasma->world_map = NewArray( struct plasma_patch *, plasma->map_height * plasma->map_width );
	MemSet( plasma->world_map, 0, sizeof( POINTER ) * plasma->map_height * plasma->map_width );
	{
		RCOORD map_seed[4];
		map_seed[0] = plasma->world_height_map[plasma->map_height/2 * plasma->map_width + plasma->map_width/ 2];
		map_seed[1] = plasma->world_height_map[plasma->map_height/2 * plasma->map_width + (1+plasma->map_width/ 2)];
		map_seed[2] = plasma->world_height_map[(1+plasma->map_height/2) * plasma->map_width + plasma->map_width/ 2];
		map_seed[3] = plasma->world_height_map[(1+plasma->map_height/2) * plasma->map_width + (1+plasma->map_width/ 2)];
		patch = PlasmaCreatePatch( plasma, map_seed, roughness );
	}

	plasma->root_x = plasma->map_height / 2;
	plasma->root_y = plasma->map_width / 2;
	patch->x = 0;
	patch->y = 0;

	plasma->world_map[ ( plasma->root_x + patch->x ) + ( plasma->root_y + patch->y ) * plasma->map_width ] = patch;

	//if( initial_render )
	{
		PlasmaRender( patch, patch->corners );
		plasma->clip.top = patch->max_height;
		plasma->clip.bottom = patch->min_height;
	}

	return patch;
}

struct plasma_patch *PlasmaCreate( RCOORD seed[4], RCOORD roughness, int width, int height )
{
	return PlasmaCreateEx( seed, roughness, width, height, TRUE );
}

struct plasma_patch *GetMapCoord( struct plasma_state *plasma, int x, int y )
{
	if( ( plasma->root_x + x ) < 0 ) 
		return NULL;
	if( ( plasma->root_y + y ) < 0 ) 
		return NULL;
	if( ( plasma->root_x + x ) >= plasma->map_width ) 
		return NULL;
	if( ( plasma->root_y + y ) >= plasma->map_height ) 
		return NULL;
	return 	plasma->world_map[ ( plasma->root_x + x ) + ( plasma->root_y + y ) * plasma->map_width ];

}

void SetMapCoord( struct plasma_state *plasma, struct plasma_patch *patch )
{
	struct plasma_patch *old_patch;
	plasma->world_map[ ( plasma->root_x + patch->x ) + ( plasma->root_y + patch->y ) * plasma->map_width ] = patch;
	if( old_patch = GetMapCoord( plasma, patch->x-1, patch->y ) )
	{
		lprintf( "%d,%d is right of %d,%d", patch->x, patch->y, old_patch->x, old_patch->y );
		patch->as_left = old_patch;
		old_patch->as_right = patch;
	}

	if( old_patch = GetMapCoord( plasma, patch->x+1, patch->y ) )
	{
		lprintf( "%d,%d is left of %d,%d", patch->x, patch->y, old_patch->x, old_patch->y );
		patch->as_right = old_patch;
		old_patch->as_left= patch;
	}

	// old patch is (on top of patch if +1 )
	if( old_patch = GetMapCoord( plasma, patch->x, patch->y+1 ) )
	{
		lprintf( "%d,%d is below %d,%d", patch->x, patch->y, old_patch->x, old_patch->y );
		patch->as_top = old_patch;
		old_patch->as_bottom = patch;
	}

	if( old_patch = GetMapCoord( plasma, patch->x, patch->y-1 ) )
	{
		lprintf( "%d,%d is above %d,%d", patch->x, patch->y, old_patch->x, old_patch->y );
		patch->as_bottom = old_patch;
		old_patch->as_top = patch;
	}

	{
		size_t stride = plasma->stride;
		size_t rows = plasma->rows;
		size_t n;
		if( old_patch = patch->as_left )
		{
			lprintf( "patch has a left..." );
			for( n = 0; n < rows; n++ )
				patch->map1[ 0 + n * stride ] = old_patch->map1[ (stride-1) + n * stride ];
		}
		if( old_patch = patch->as_top )
		{
			lprintf( "patch has a top..." );
			for( n = 0; n < stride; n++ )
				patch->map1[ n + 0 * stride ] = old_patch->map1[ n + (rows-1) * stride ];
		}

		if( old_patch = patch->as_right )
		{
			lprintf( "patch has a right..." );
			for( n = 0; n < rows; n++ )
				patch->map1[ (stride-1) + n * stride ] = old_patch->map1[ 0 + n * stride ];
		}

		if( old_patch = patch->as_bottom )
		{
			lprintf( "patch has bottom... (%d,%d) above(%d,%d)", patch->x, patch->y, old_patch->x, old_patch->y );
			for( n = 0; n < stride; n++ )
				patch->map1[ n + (rows-1) * stride ] = old_patch->map1[ n + 0 * stride ];
		}
	}
}

struct plasma_patch *PlasmaExtend( struct plasma_patch *plasma, int in_direction, RCOORD seed[2], RCOORD roughness )
{
	struct plasma_patch *new_plasma;
	RCOORD new_seed[4];
	switch( in_direction )
	{
	case 0: // to the right
		if( plasma->as_right )
			return plasma->as_right;
		new_seed[0] = plasma->corners[1];
		new_seed[1] = plasma->plasma->world_height_map[ ( plasma->x + plasma->plasma->root_x + 1 ) + (plasma->y + plasma->plasma->root_y) * plasma->plasma->map_width ];
		new_seed[2] = plasma->corners[3];
		new_seed[3] = plasma->plasma->world_height_map[ ( plasma->x + plasma->plasma->root_x + 1 ) + (plasma->y + plasma->plasma->root_y + 1) * plasma->plasma->map_width ];
		break;
	case 1: // to the bottom
		if( plasma->as_bottom )
			return plasma->as_bottom;
		new_seed[0] = plasma->corners[2];
		new_seed[1] = plasma->corners[3];
		new_seed[2] = plasma->plasma->world_height_map[ ( plasma->x + plasma->plasma->root_x ) + (plasma->y + plasma->plasma->root_y + 1) * plasma->plasma->map_width ];
		new_seed[3] = plasma->plasma->world_height_map[ ( plasma->x + plasma->plasma->root_x + 1 ) + (plasma->y + plasma->plasma->root_y + 1) * plasma->plasma->map_width ];
		break;
	case 2: // to the left
		if( plasma->as_left )
			return plasma->as_left;
		new_seed[0] = plasma->plasma->world_height_map[ ( plasma->x + plasma->plasma->root_x - 1) + (plasma->y + plasma->plasma->root_y) * plasma->plasma->map_width ];
		new_seed[1] = plasma->corners[0];
		new_seed[2] = plasma->plasma->world_height_map[ ( plasma->x + plasma->plasma->root_x - 1) + (plasma->y + plasma->plasma->root_y + 1) * plasma->plasma->map_width ];
		new_seed[3] = plasma->corners[2];
		break;
	case 3: // to the top
		if( plasma->as_top )
			return plasma->as_top;
		new_seed[0] = plasma->plasma->world_height_map[ ( plasma->x + plasma->plasma->root_x ) + (plasma->y + plasma->plasma->root_y-1) * plasma->plasma->map_width ];
		new_seed[1] = plasma->plasma->world_height_map[ ( plasma->x + plasma->plasma->root_x + 1 ) + (plasma->y + plasma->plasma->root_y-1) * plasma->plasma->map_width ];
		new_seed[2] = plasma->corners[0];
		new_seed[3] = plasma->corners[1];
		break;
	}

	new_plasma = PlasmaCreatePatch( plasma->plasma, new_seed, roughness );

	switch( in_direction )
	{
	case 0: // to the right
		new_plasma->x = plasma->x + 1;
		new_plasma->y = plasma->y;
		break;
	case 1: // to the bottom
		new_plasma->x = plasma->x;
		new_plasma->y = plasma->y - 1;
		break;
	case 2: // to the left
		new_plasma->x = plasma->x - 1;
		new_plasma->y = plasma->y;
		break;
	case 3: // to the top
		new_plasma->x = plasma->x;
		new_plasma->y = plasma->y + 1;
		break;
	}
	lprintf( "Create plasma at %d,%d", new_plasma->x, new_plasma->y );
	// overwrites the corners...
	SetMapCoord( plasma->plasma, new_plasma );

	{
		size_t stride = plasma->plasma->stride;
		size_t rows = plasma->plasma->rows;
		if( new_plasma->as_top || new_plasma->as_left )
			new_plasma->corners[0] = new_plasma->map1[0 + 0 * stride];
		if( new_plasma->as_top || new_plasma->as_right )
			new_plasma->corners[1] = new_plasma->map1[(stride - 1) + 0 * stride];
		if( new_plasma->as_bottom || new_plasma->as_left )
			new_plasma->corners[2] = new_plasma->map1[0 + (rows-1) * stride];
		if( new_plasma->as_bottom || new_plasma->as_right )
			new_plasma->corners[3] = new_plasma->map1[(stride - 1) + (rows-1) * stride];
	}

	PlasmaRender( new_plasma, new_plasma->corners );

	return new_plasma;
}


RCOORD *PlasmaGetSurface( struct plasma_patch *plasma )
{
	return plasma->map;
}

#ifdef _MSC_VER
__inline 
#endif
	RCOORD GetMapData( struct plasma_patch *patch, int x, int y, int smoothing, int force_scaling )
{
		RCOORD input = patch->map1[ x + y * patch->plasma->stride ];
		RCOORD top = patch->plasma->clip.top;// patch->_max_height;
		RCOORD bottom = patch->plasma->clip.bottom; // patch->_min_height;
		//lprintf( "patch %p (%d,%d) = %g", patch, x, y, input );
		if( force_scaling /*( patch->min_height < patch->_min_height || patch->max_height > patch->_max_height )*/ )
		{
			int tries = 0;
			LOGICAL updated;
			do
				{
					tries++;
					if( tries > 5 )
					{
						//lprintf( "capping oscillation at 20" );
						break;
					}
					updated  = FALSE;
					if( input > top )
					{
						// 
						input = top - ( input - top );
						updated = TRUE;
					}
					else if( input < bottom )
					{
						input = bottom - ( input - bottom );
						updated = TRUE;
					}
				}
				while( updated );
				input = ( input - bottom ) / ( top - bottom );
				if( smoothing == 0 )
				{
				// need to specify a copy mode.
					return input;
					//sin( /*map2[index] * */( map_from[0] - patch->min_height ) / divider * 3.14159/2 /*+ map2[index] * 3.14159/2*/ );
					//( 1 + sin( /*map2[index] * */( map_from[0] - patch->min_height ) / divider * 3.14159 - 3.14159/2 /*+ map2[index] * 3.14159/2*/ ) ) / 2;
				}

				else if( smoothing == 1 )
				{
					// smooth top and bottom across sin curve, middle span is 1:1 ...
					return
						( 1 + sin( input * 3.14159 - 3.14159/2 ) ) / 2;
				}

				if( smoothing == 3 )  // bad mode ... needs work
				{
				// peaker tops and bottoms smoother middle, middle span ...
				return 
					( 1 + tan( ( ( input ) + 0.5 ) * ( 3.14159 * 0.5 ) + (3.14159/2) ) ) /2;
				}

				if( smoothing == 4 ) // use square furnction, parabolic... cubic... qudric?
				// peaker tops and bottoms smoother middle, middle span is 1:1 ...
				{
					RCOORD tmp = input - 0.5;
					if( tmp < 0 )
						return
							( 0.5 + ( 2 * tmp * tmp ) );
					else
						return
							0.5 -( 2 * tmp * tmp );
				}
				//*/
				//lprintf( "%g = %g %g", patch->map[index], patch->map1[index], map2[index] );
		}
		else
		{
			RCOORD divider = ( patch->max_height - patch->min_height );
			{
				// need to specify a copy mode.
				return ( input - patch->min_height ) / divider;
					//sin( /*map2[index] * */( map_from[0] - patch->min_height ) / divider * 3.14159/2 /*+ map2[index] * 3.14159/2*/ );
					//( 1 + sin( /*map2[index] * */( map_from[0] - patch->min_height ) / divider * 3.14159 - 3.14159/2 /*+ map2[index] * 3.14159/2*/ ) ) / 2;

				/*
				// smooth top and bottom across sin curve, middle span is 1:1 ...
				map[0] = 
					( 1 + sin( ( map_from[0] - patch->min_height ) / divider * 3.14159 - 3.14159/2 ) ) / 2;
				*/
				///*
				// peaker tops and bottoms smoother middle, middle span ...
				//map[0] = 
				//	( 1 + tan( ( ( ( map_from[0] - patch->min_height ) / divider ) + 0.5 ) * ( 3.14159 * 0.5 ) + (3.14159/2) ) ) /2;
				// peaker tops and bottoms smoother middle, middle span is 1:1 ...
				if( 0 )
				{
					RCOORD tmp = ( input - patch->min_height ) / divider - 0.5;
					if( tmp < 0 )
						return ( 0.5 + ( 2 * tmp * tmp ) );
					else
						return 0.5 -( 2 * tmp * tmp );
				}
			}
		}
		return 0;
}


RCOORD *PlasmaReadSurface( struct plasma_patch *patch_root, int x, int y, int smoothing, int force_scaling )
{
	struct plasma_patch *first_patch;
	struct plasma_patch *last_patch;
	struct plasma_patch *patch;
	struct plasma_state *plasma = patch_root->plasma;
	int del_x = x < 0 ? - ((int)plasma->stride-1) : ((int)0);
	int del_y = y < 0 ? - ((int)plasma->rows-1) : ((int)0);
	int sec_x = (x +del_x) / (int)plasma->stride;
	int sec_y = -(y +del_y) / (int)plasma->rows;

	int ofs_x = (x) % plasma->stride;
	int ofs_y = (y) % plasma->rows;
	int out_x, out_y;
	lprintf( "start at %d,%d  offset in first: %d,%d  sec: %d,%d", x, y, ofs_x, ofs_y, sec_x, sec_y );

	first_patch
		= patch 
		= GetMapCoord( plasma, sec_x, sec_y );
	
	if( !first_patch )
	{
		do
		{
			int n, m;
			patch = patch_root;
			while( sec_x < patch->x )
			{
				RCOORD seed[2];
				seed[0] = 0.5;
				seed[1] = 0.5;
				patch = PlasmaExtend( patch, 2, seed, patch_root->area_scalar );
			}
			while( sec_y < patch->y )
			{
				RCOORD seed[2];
				seed[0] = 0.5;
				seed[1] = 0.5;
				patch = PlasmaExtend( patch, 1, seed, patch_root->area_scalar );
			}
			while( sec_x > patch->x )
			{
				RCOORD seed[2];
				seed[0] = 0.5;
				seed[1] = 0.5;
				patch = PlasmaExtend( patch, 0, seed, patch_root->area_scalar );
			}
			while( sec_y > patch->y )
			{
				RCOORD seed[2];
				seed[0] = 0.5;
				seed[1] = 0.5;
				patch = PlasmaExtend( patch, 3, seed, patch_root->area_scalar );
			}
			patch = GetMapCoord( plasma, sec_x, sec_y );
			if( !patch )
			{
				lprintf( "Failed to get the patch... %d,%d", sec_x, sec_y );
				return NULL;
			}
		}
		while( !( patch = GetMapCoord( plasma, sec_x, sec_y ) ) );
		first_patch = patch;
	}
	lprintf( "patch is %d,%d", patch->x, patch->y ) ;
	if( patch )
	for( out_x = ofs_x; out_x < plasma->stride; out_x++ )
		for( out_y = ofs_y; out_y < plasma->rows; out_y++ )
		{
			plasma->read_map[ ( out_x - ofs_x ) + ( out_y - ofs_y ) * plasma->stride] 
				= GetMapData( patch, out_x, out_y, smoothing, force_scaling );
		}
	if( ofs_x && ( ( plasma->root_x + sec_x + 1 ) < plasma->map_width ) )
	{
		patch = GetMapCoord( plasma, sec_x + 1, sec_y );
		if( !patch )
		{
			RCOORD seed[2];
			seed[0] = 0.5;
			seed[1] = 0.5;
			patch = PlasmaExtend( first_patch, 0, seed, first_patch->area_scalar );
		}
		lprintf( "patch is %d,%d", patch->x, patch->y ) ;
		if( patch )
		for( out_x = 0; out_x < ofs_x; out_x++ )
			for( out_y = ofs_y; out_y < plasma->rows; out_y++ )
			{
				plasma->read_map[ ( out_x + ( plasma->stride - ofs_x ) ) + ( out_y - ofs_y ) * plasma->stride] 
					= GetMapData( patch, out_x, out_y, smoothing, force_scaling );
			}
	}
	if( ( plasma->root_y + sec_y - 1 ) >= 0 )
	{
		patch = GetMapCoord( plasma, sec_x, sec_y - 1 );
		if( !patch )
		{
			RCOORD seed[2];
			seed[0] = 0.5;
			seed[1] = 0.5;
			patch = PlasmaExtend( first_patch, 1, seed, first_patch->area_scalar );
		}
		lprintf( "patch is %d,%d", patch->x, patch->y ) ;
		if( patch )
		for( out_x = ofs_x; out_x < plasma->stride; out_x++ )
			for( out_y = 0; out_y < ofs_y; out_y++ )
			{
				plasma->read_map[ ( out_x - ofs_x ) + ( out_y + ( plasma->rows - ofs_y ) ) * plasma->stride] 
					= GetMapData( patch, out_x, out_y, smoothing, force_scaling );
			}
	}
	last_patch = patch;
	if( ( ( plasma->root_x + sec_x + 1 ) < plasma->map_width ) && ( ( plasma->root_y + sec_y - 1 ) >= 0 ) )
	{
		patch = GetMapCoord( plasma, sec_x + 1, sec_y - 1 );
		if( !patch )
		{
			RCOORD seed[2];
			seed[0] = 0.5;
			seed[1] = 0.5;
			patch = PlasmaExtend( last_patch, 0, seed, first_patch->area_scalar );
		}
		lprintf( "patch is %d,%d", patch->x, patch->y ) ;
		if( patch )
		for( out_x = 0; out_x < ofs_x; out_x++ )
			for( out_y = 0; out_y < ofs_y; out_y++ )
			{
				plasma->read_map[ ( out_x + ( plasma->stride - ofs_x ) ) + ( out_y + ( plasma->rows - ofs_y ) ) * plasma->stride] 
					= GetMapData( patch, out_x, out_y, smoothing, force_scaling );
			}
	}
	return plasma->read_map;
}

void PlasmaSetRoughness( struct plasma_patch *plasma, RCOORD roughness, RCOORD horiz_rough )
{
	plasma->area_scalar = roughness;
	plasma->horiz_area_scalar = roughness * horiz_rough;
}

void PlasmaSetGlobalRoughness( struct plasma_patch *plasma, RCOORD roughness, RCOORD horiz_rough )
{
	struct plasma_state *state = plasma->plasma;
	int x, y;
	struct plasma_patch **here = state->world_map;

	for( y = 0; y < state->map_height; y++ )
		for( x = 0; x < state->map_width; x++ )
		{
			if( here[0] )
			{
				(*here)->area_scalar = roughness;
				(*here)->horiz_area_scalar = roughness;
			}
			here++;
		}
}


struct plasma_state *PlasmaGetMap( struct plasma_patch *plasma )
{
	return plasma->plasma;
}

void PlasmaSetMasterMap( struct plasma_state *plasma, RCOORD *master_map, int width, int height )
{

}
