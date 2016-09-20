#define DEFINE_DEFAULT_RENDER_INTERFACE
#include <virtuality/view.h>
#include <world.h>

static struct {
   INDEX world;
} l;

typedef struct player_body BODY, *PBODY;

struct DrawInfo{
	INDEX iWorld;
	POBJECT object;
	POBJECT root;
} DrawThis;

uintptr_t CPROC DrawSectorLines( INDEX pSector, uintptr_t unused )
{
	_POINT o, n;
	POBJECT object = CreateObject();
	INDEX iWorld = DrawThis.iWorld;
	INDEX pStart, pCur;
	int count=0;
	int priorend = unused;
	DrawThis.object = object;
		// origin at z +1
		scale( o, VectorConst_Y, 10.0 );//SetPoint( o, VectorConst_Z );
		// direction z+1
		SetPoint( n, VectorConst_Y );
		AddPlane( object, o, n, 0 );
		Invert( n );
		Invert( o );
		AddPlane( object, o, n, 0 );
		//DrawSpaceLines( display->pImage, pSector->spacenode );
		lprintf( WIDE("Adding Sector %d"), pSector );
	pCur = pStart = GetFirstWall( iWorld, pSector, &priorend );
	do
	{
		_POINT tmp;
      LOGICAL d = IsSectorClockwise( iWorld, pSector );
      //GETWORLD( display->pWorld );
      INDEX iLine;
		PFLATLAND_MYLINESEG Line;
		//VECTOR p1, p2;

      iLine = GetWallLine( iWorld, pCur );

		GetLineData( iWorld, iLine, &Line );
      // stop infinite looping... may be badly linked...
		count++;
		if( count > 20 )
		{
         xlprintf(LOG_ALWAYS)( WIDE("Conservative limit of 20 lines on a wall has been reached!") );
         DebugBreak();
			break;
		}
		{
			_POINT o, n, r;
			//int d;
         o[0] = Line->r.o[0];
         o[1] = Line->r.o[2];
         o[2] = Line->r.o[1];
         tmp[0] = Line->r.n[0];
         tmp[1] = Line->r.n[2];
         tmp[2] = Line->r.n[1];
			if( priorend )
			{
            lprintf( WIDE("prior end causes reversal...") );
				Invert( tmp );
			}
			if( d )
			{
            lprintf( WIDE("Sector clockwise..>") );
				Invert( tmp );
			}
			//SetPoint( o, Line->r.o );
			crossproduct( n, VectorConst_Y, tmp );
         /*
			if( priorend )
			{
            d = 1;
				Invert( n );
			}
			else
			{
				d = 1;
				}
            */
			lprintf( WIDE("Adding plane to object... ") );
			PrintVector( o );
			PrintVector( n );
			AddPlane( DrawThis.object, o, n, (GetMatedWall( iWorld, pCur )==INVALID_INDEX)?0:2 );
		}

		pCur = GetNextWall(l.world
								, pCur, &priorend );
	}while( pCur != pStart );
	if( IntersectObjectPlanes( object ) )
	{
		// destroy these planes, and add new ones.
		//priorend = FALSE;
      //DestroyObject( &object );
	}
	// oh - add lines first one way and then the other... interesting
	// to test both directions of priorend!
	if( !unused )
		DrawSectorLines( pSector, 1 );
	PutIn( object, DrawThis.root );
	{
		PFLATLAND_TEXTURE texture;
		INDEX iTexture = GetSectorTexture( l.world, pSector );
		GetTextureData( l.world, iTexture, &texture );
		SetObjectColor( object, texture->data.color /*BASE_COLOR_BLUE*/ );
	}

	return 0; // continue drawing... non zero breaks loop...
}



PBODY EnterWorld( INDEX iWorld )
{
	POBJECT root = CreateObject();
	{
		RCOORD size;
		CDATA c;
		{
			DrawThis.iWorld = iWorld;
			DrawThis.root = root;
			ForAllSectors( iWorld, DrawSectorLines, 0 );
		}
	}
	SetRootObject( root );
	return NULL;
}

int main( int argc, char **argv )
{
	l.world = OpenWorld( WIDE("") );
	LoadWorldFromFile( l.world );
	WakeableSleep( 500 ); // give the world time to load.
	
	{
		uint32_t w, h;
		PBODY body = EnterWorld( l.world );
		PVIEW view ;
		GetDisplaySizeEx( 0, NULL,NULL,&w, &h );
		//view = CreateViewEx( V_FORWARD, NULL, WIDE("blah"), h/3, h/3 );
		//view = CreateViewEx( V_DOWN, NULL, WIDE("blah"), h/3, 0 );
		//view = CreateViewEx( V_UP, NULL, WIDE("blah"), h/3, 2*h/2 );
		//view = CreateViewEx( V_RIGHT, NULL, WIDE("blah"), 0, h/3 );
		//view = CreateViewEx( V_LEFT, NULL, WIDE("blah"), 2*h/3, h/3 );
		//view = CreateViewEx( V_BEHIND, NULL, WIDE("blah"), 3*h/3, h/3 );
		//Equip( body );
		//PlayWorld( world, body );
		while( 1 )
			Sleep( 1000 );
	}
	return 0;
}
