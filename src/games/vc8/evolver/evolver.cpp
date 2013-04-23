
#include <stdhdrs.h>
#include <controls.h>

#include <psi.h>
#include <vectlib.h>
#include <timers.h>

#include "../../automaton/brain/brain.hpp"


// forward declaraion...
extern int CPROC MyClassDrawThing( PSI_CONTROL pc );
extern int CPROC MyClassDrawThing( PSI_CONTROL pc );

static struct {
	PLIST registered;
} l;

static CONTROL_REGISTRATION *FindRegistration( char *name )
{
	INDEX idx;
	CONTROL_REGISTRATION *x;
	LIST_FORALL( l.registered, idx, CONTROL_REGISTRATION*, x )
	{
		if( strcmp( x->name, name ) == 0 )
			break;
	}
	return x;
}

class PSI_SimpleControl {
private:
	CONTROL_REGISTRATION *ControlType;
	P_32 _MyControlID;
public:
	PSI_CONTROL pc;
	PSI_SimpleControl( PSI_CONTROL parent, char *name );
	PSI_SimpleControl( PSI_CONTROL parent, INDEX type );
	virtual int OnDraw( PSI_CONTROL pc ) = 0;
	virtual int OnMouse( PSI_CONTROL pc, S_32, S_32, _32 ) = 0;
};




int CPROC MyClassDrawThing( PSI_CONTROL pc )
{
	PSI_SimpleControl* _this = ControlData( PSI_SimpleControl*, pc );
	return _this->OnDraw( pc );
}

int CPROC MyClassMouseThing( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
{
	PSI_SimpleControl* _this = ControlData( PSI_SimpleControl*, pc );
	return _this->OnMouse( pc, x, y, b );
}



PSI_SimpleControl::PSI_SimpleControl( PSI_CONTROL parent, char *name )
{
	char buffer[256];
	ControlType = FindRegistration( name );
	if( !ControlType )
	{
		ControlType= New( CONTROL_REGISTRATION );
		MemSet( ControlType, 0, sizeof( CONTROL_REGISTRATION ) );
		ControlType->name = name;
		ControlType->stuff.stuff.width = 256;
		ControlType->stuff.stuff.height = 256;
		ControlType->stuff.extra = 0;
		ControlType->stuff.default_border = BORDER_THINNER;
		DoRegisterControl( ControlType );
		AddLink( &l.registered, ControlType ); 
		_MyControlID = &ControlType->TypeID;

		snprintf( buffer, sizeof( buffer ), "psi/control/%s/rtti", name );
		SimpleRegisterMethod( buffer, MyClassDrawThing 
	                    , "int", "draw", "(PSI_CONTROL)" );
		snprintf( buffer, sizeof( buffer ), "psi/control/%s/rtti", name );
		SimpleRegisterMethod( buffer, MyClassMouseThing 
	                    , "int", "mouse", "(PSI_CONTROL,S_32,S_32,_32)" );
	}
	pc = MakeNamedControl( parent, name, 0, 0, 1024, 768, -1 );
	((PSI_SimpleControl**)(pc))[0] = this;
}


typedef struct grid_marks {
    int GridXUnits; // Major division lines here...
    int GridYUnits;// basic grid unit here - X * these is total distance of majors
    int GridXSets;    // number of minor divisions between major
    int GridYSets;	  // number of minor divisions between major
    CDATA GridColor;
    CDATA Background;
};


struct display_IO {
	
    int delxaccum, delyaccum;
    int x, y, b; // last state of mouse

    _POINT origin; // offset of origin to display...
    RCOORD scale; // 1:1 = default 
                  // 1:100 - small changes make big worlds 
                      // 1:0.001 - big changes make small worlds
    int xbias, ybias, zbias;  // bias to make origin be centered on display
                              // zbias is more of a zoom factor...


};

typedef struct display_tag: public display_IO, public grid_marks {
	struct {
		unsigned int bShiftMode : 1;
        unsigned int bDragging           : 1; // something dragging...
        unsigned int bLocked             : 1; // locked on something.

            // only one of these may be true
            // and these are only valid if bDragging or bLocked is set.
        unsigned int bDisplay           : 1; // (dragging only) move display
        unsigned int bSectorOrigin      : 1; // origin of sector
        unsigned int bOrigin                : 1; // orogin of wall
        unsigned int bSlope             : 1; // using slope of line to update
        unsigned int bEndStart      : 1; // end of wall (start)
        unsigned int bEndEnd          : 1; // end of wall (end)

        unsigned int bSelect                    : 1;
      unsigned int bMarkingMerge        : 1;

        unsigned int bSectorList            : 1;
        unsigned int bWallList              : 1;

        unsigned int bShowSectorText        : 1;
        unsigned int bShowLines             : 1;
        unsigned int bShowSectorTexture : 1;
        unsigned int bUseGrid               : 1;
        unsigned int bShowGrid              : 1;
        unsigned int bGridTop               : 1; 
        unsigned int bDrawNearSector     : 1; // draw a range of sectors from current.
		unsigned int bNormalMode  : 1; // edit normals, not wall position...
    } flags;

	/* so far these above are only things used... */

    INDEX  pWorld;    // what world we're showing now...

    int     nSectors;  // number of selected sectors
    struct {
        INDEX Sector;
        INDEX *SectorList; // may be &CurSector.Sector ...
    } CurSector;

    int nWalls;
    struct {
        INDEX   Wall;   // current selected wall...
        INDEX *WallList;
    } CurWall;

    _POINT SectorOrigin;
    int CurSecOrigin[2];
    _POINT WallOrigin;
    int CurOrigin[2];
    int CurEnds[2][2];
    int CurSlope[2];
	 ORTHOAREA SelectRect;

    INDEX MarkedWall;
    _POINT lastpoint; // save last point - though mouse moved, point might not

    PRENDERER hVideo;
    Image  pImage;
} DISPLAY, *PDISPLAY;

#define ONE_SCALE   1
#define DISPLAY_SCALE(pdisplay, c)    ( ( ( (c) / (pdisplay)->scale ) * ONE_SCALE ) / (pdisplay)->zbias )
#define DISPLAY_X(pdisplay, x)     ( DISPLAY_SCALE( (pdisplay), (x) + (pdisplay)->origin[0] ) + (pdisplay)->xbias )
#define DISPLAY_Y(pdisplay, y)     ( DISPLAY_SCALE( (pdisplay), (pdisplay)->origin[1] - (y) ) + (pdisplay)->ybias )

#define REAL_SCALE(pdisplay, c) (RCOORD)( ( (RCOORD)(c) * (RCOORD)(pdisplay)->zbias * (pdisplay)->scale ) / (RCOORD)ONE_SCALE )
#define REAL_X(pdisplay,x)        ( REAL_SCALE( (pdisplay), (x) - (pdisplay)->xbias ) - (pdisplay)->origin[0] )
#define REAL_Y(pdisplay,y)        ( REAL_SCALE( (pdisplay), (pdisplay)->ybias - (y) ) + (pdisplay)->origin[1] )



void DrawDisplayGrid( PDISPLAY display )
{
	int x, y;
	//maxx, maxy, miny,, incx, incy, delx, dely, minx
	_32 start= GetTickCount();;

		int DrawSubLines, DrawSetLines;
		// , drawn
		RCOORD units, set;
		int drawnSet = FALSE, drawnUnit = FALSE;
		DrawSubLines = DrawSetLines = TRUE;
		units = ((RCOORD)display->GridXUnits) * display->scale;
		if( DISPLAY_SCALE( display, display->GridXUnits ) < 5 )
			DrawSubLines = FALSE;
		set = ((RCOORD)( display->GridXUnits * display->GridXSets )) * display->scale;
		if( DISPLAY_SCALE( display, display->GridXUnits * display->GridXSets ) < 5 )
			DrawSetLines = FALSE;
			
		for( x = 0; x < display->pImage->width; x++ )
		{
			RCOORD real = REAL_X( display, x );
			RCOORD nextreal = REAL_X( display, x+1 );
			int setdraw, unitdraw;
			if( real > 0 )
			{
				setdraw = ( (int)(nextreal/set)==((int)(real/set)));
				unitdraw = ( (int)(nextreal/units)==((int)(real/units)) );
				//Log7( "test: %d %d %d %g %g %d %d", setdraw, unitdraw, x, nextreal, units, (int)(nextreal/units), (int)(real/units) );
			}
			else
			{
				setdraw = ( ((int)(nextreal/set))!=((int)(real/set)) );
				unitdraw = ( ((int)(nextreal/units))!=((int)(real/units)) );
				//Log7( "test: %d %d %d %g %g %d %d", setdraw, unitdraw, x, nextreal, units, (int)(nextreal/units), (int)(real/units) );
			}

			if( !setdraw )
				drawnSet = FALSE;
			if( !unitdraw )
				drawnUnit = FALSE;

			if( (real <= 0) && (nextreal > 0) )
			{
				do_vlineAlpha( display->pImage
								, x
								, 0, display->pImage->height
								, AColor( 255, 255, 255, 64 ) );
				drawnUnit = TRUE;
				drawnSet = TRUE;
			}
			else if( DrawSetLines && setdraw && !drawnSet )
			{
				do_vlineAlpha( display->pImage
								, x
								, 0, display->pImage->height
								, SetAlpha( display->GridColor, 96 ) );
				drawnSet = TRUE;
				drawnUnit = TRUE;
			}
			else if( DrawSubLines && unitdraw && !drawnUnit)
			{
				do_vlineAlpha( display->pImage
								, x
								, 0, display->pImage->height
								, SetAlpha( display->GridColor, 48 ) );
				drawnUnit = TRUE;
			}
		}
		//if( dotiming ) 
		//{
//			Log3( "%s(%d): %d Vertical Grid", __FILE__, __LINE__, GetTickCount() - start );
//			start = GetTickCount();
//		}  

		drawnSet = drawnUnit = FALSE;
		DrawSubLines = DrawSetLines = TRUE;

		units = ((RCOORD)display->GridYUnits) * display->scale;
		if( DISPLAY_SCALE( display, display->GridYUnits ) < 5 )
			DrawSubLines = FALSE;
		set = ((RCOORD)( display->GridYUnits * display->GridYSets )) * display->scale;
		if( DISPLAY_SCALE( display, display->GridYUnits * display->GridYSets ) < 5 )
			DrawSetLines = FALSE;

		for( y = display->pImage->height - 1; y >= 0 ; y-- )
		//for( y = 0; y < display->pImage->height ; y++ )
		{
			RCOORD real = REAL_Y( display, y );
			RCOORD nextreal = REAL_Y( display, y-1 );
			//RCOORD nextreal = REAL_Y( display, y+1 );
			int setdraw, unitdraw;
			if( real > 0 )
			{
				setdraw = ( (int)(nextreal/set)==((int)(real/set)));
				unitdraw = ( (int)(nextreal/units)==((int)(real/units)) );
			}
			else
			{
				setdraw = ( ((int)(nextreal/set))!=((int)(real/set)) );
				unitdraw = ( ((int)(nextreal/units))!=((int)(real/units)) );
			}
			if( !setdraw )
				drawnSet = FALSE;
			if( !unitdraw )
				drawnUnit = FALSE;

			if( (real <= 0) && (nextreal > 0) )
			{
				do_hlineAlpha( display->pImage
								, y
								, 0, display->pImage->width
								, AColor( 255, 255, 255, 64 ) );
				drawnUnit = TRUE;
				drawnSet = TRUE;
			}
			else if( DrawSetLines && setdraw && !drawnSet )
			{
				do_hlineAlpha( display->pImage
								, y
								, 0, display->pImage->width
								, SetAlpha( display->GridColor, 96 ) );
				drawnSet = TRUE;
				drawnUnit = TRUE;
			}
			else if( DrawSubLines && unitdraw && !drawnUnit)
			{
				do_hlineAlpha( display->pImage
								, y
								, 0, display->pImage->width
								, SetAlpha( display->GridColor, 48 ) );
				drawnUnit = TRUE;
			}
		}
//		if( dotiming )
///		{
//			Log3( "%s(%d): %d Horizontal Grid", __FILE__, __LINE__, GetTickCount() - start );
//			start = GetTickCount();
//		}
}  


class collider {
	collider()
	{
	}
};

void WireRandomBrain( PBRAIN brain )
{
}

struct actor
{
	TRANSFORM transform; // current orientation in space - allows for rotation
	PC_POINT p;
	RCOORD space; // the radius of space this occupies.

	_32 lifespan;
	S_32 health;
	_32 offense;

	NATIVE want_look_left;
	NATIVE want_look_right;

	NATIVE want_move_forward;
	NATIVE want_move_backward;
	NATIVE want_turn_left;
	NATIVE want_turn_right;

	NATIVE nearest_forward;
	NATIVE nearest;
	NATIVE nearest_eye;
	NATIVE eye_angle; // rotated by want_look_left/right

	BRAIN *brain;
	PLIST *actor_list;
	struct actor **me;
public:
	~actor( )
	{
		// should destroy all attached brainstems, connectors...
		INDEX idx = FindLink( actor_list, this );
		if( idx != INVALID_INDEX )
			SetLink( actor_list, idx, NULL );
		delete brain;
	}
	actor( PLIST *what_list, int x, int y )
	{
		ClearTransform( &transform );
		Translate( &transform, x, y, 0 );
		want_look_left = 0;
		want_look_right = 0;

		want_move_forward = 0;
		want_move_backward = 0;
		want_turn_left = 0;
		want_turn_right = 0;

		nearest_forward = 0;
		nearest = 0;
		nearest_eye = 0;
		eye_angle = 0; // rotat
		lifespan = 0;
		health = 0;
		offense = 0;

		{
#define NEURON_DEFAULT 100
#define CONNECTIONS 300
			PNEURON neurons[NEURON_DEFAULT];
			int n;
			brain = new BRAIN();
			brain->Stop();
			for( n = 0; n < NEURON_DEFAULT; n++ )
			{
				neurons[n] = brain->GetNeuron();
				neurons[n]->set( 0, 1000, ( rand() * NEURON_DEFAULT ) / (RAND_MAX+1) );
			}
			for( n = 0; n < CONNECTIONS; n++ )
			{
				int r;
				PSYNAPSE wire = brain->GetSynapse();
				if( wire && !wire->AttachSource( neurons[ r =( rand() * NEURON_DEFAULT ) / (RAND_MAX+1) ] ) ){ brain->ReleaseSynapse( wire ); wire = NULL; }
				if( wire && !wire->AttachDestination( neurons[r = ( rand() * NEURON_DEFAULT ) / (RAND_MAX+1) ] ) ){ brain->ReleaseSynapse( wire ); wire = NULL; }
				if( wire ) wire->set( 256 - ( ( rand() * 513 ) / RAND_MAX ) );
			}
			{
			connector *conn;
			PNEURON tmp;
			PBRAIN_STEM pbs = new BRAIN_STEM("actor");
			brain->AddBrainStem( pbs );

			PBRAIN_STEM pbs2 = new BRAIN_STEM("Look");

			pbs2->AddOutput( conn = new connector( "Left", &want_look_left ) );
#define AddRandomAttachedInput()  			{											\
				tmp = brain->GetInputNeuron( conn );							 \
				PSYNAPSE wire = brain->GetSynapse();								  \
				if( wire && !wire->AttachDestination( neurons[ ( rand() * NEURON_DEFAULT ) / (RAND_MAX+1) ] ) ){ brain->ReleaseSynapse( wire ); wire = NULL; } 		   \
				if( wire && !wire->AttachSource( tmp ) ){ brain->ReleaseSynapse( wire ); wire = NULL; } 									   \
				if( wire ) wire->set( 256 - ( ( rand() * 513 ) / RAND_MAX ) );					   \
			}																		   
#define AddRandomAttachedOutput()  			{											\
				tmp = brain->GetOutputNeuron( conn );							 \
				PSYNAPSE wire = brain->GetSynapse();								  \
				if( wire && !wire->AttachSource( neurons[ ( rand() * NEURON_DEFAULT ) / (RAND_MAX+1) ] ) ) { brain->ReleaseSynapse( wire ); wire = NULL; } \
				if( wire && !wire->AttachDestination( tmp ) ) { brain->ReleaseSynapse( wire ); wire = NULL; }					   \
				if( wire ) wire->set( 256 - ( ( rand() * 513 ) / RAND_MAX ) );					   \
			}																		   
			AddRandomAttachedOutput();
			pbs2->AddOutput( conn = new connector( "Right", &want_look_right ) );
			AddRandomAttachedOutput();
			pbs->AddModule( pbs2 );

			pbs->AddModule( pbs2 = new BRAIN_STEM( "Move" ) );
			pbs2->AddOutput( conn = new connector( "Left", &want_turn_left ) );
			AddRandomAttachedOutput();
			pbs2->AddOutput( conn = new connector( "Right", &want_turn_right ) );
			AddRandomAttachedOutput();
			pbs2->AddOutput( conn = new connector( "Forward", &want_move_forward ) );
			AddRandomAttachedOutput();
			pbs2->AddOutput( conn = new connector( "Backward", &want_move_backward ) );
			AddRandomAttachedOutput();

			pbs->AddModule( pbs2 = new BRAIN_STEM( "See" ) );
			pbs2->AddInput( conn = new connector( "nearest forward", &nearest_forward ) );
			AddRandomAttachedInput();
			pbs2->AddInput( conn = new connector( "nearest", &nearest ) );
			AddRandomAttachedInput();
			pbs2->AddInput( conn = new connector( "Nearest eye", &nearest_forward ) );
			AddRandomAttachedInput();
			}
			brain->Run();
		}
		p = GetOrigin( &transform );
		// set me
		// and - this means I can process
		actor_list = what_list;
	};

	void Tick( void )
	{
		// want_move_left/right needs to be considered from min to max output...
		// what is a reasonable limit range... proportion it? clip it?
		RotateRel( &transform, 0, want_turn_right - want_turn_left, 0 );

		addscaled( (P_POINT)p, p
			, GetAxis( &transform, vForward )
			, ((want_move_forward>0)?want_move_forward:0)
				- ((want_move_backward>0)?want_move_backward:0) );
		lifespan++;
		if( lifespan > 50 )
		{
			if( want_move_forward < 20 )
			{
				delete this;
			}
		}
	}

	LOGICAL IsActorNear( actor *other )
	{
		_POINT r;
		if( Length( sub( r, other->p, p ) ) <= space )
			return TRUE;
		return FALSE;
	};
};

void DrawActors( PDISPLAY display, PLIST actors )
{
	actor *a;
	INDEX idx;
	LIST_FORALL( actors, idx, actor*, a )
	{
		S_32 x, y;
#define MARK_SIZE 3
		if( a == (actor*)1 )
			continue; // not a real entry...
		x = DISPLAY_X( display, a->p[0] );
		y = DISPLAY_Y( display, a->p[1] );
		do_line( display->pImage, x - MARK_SIZE
			, y - MARK_SIZE
			, x + MARK_SIZE
			, y - MARK_SIZE
			, Color( 200, 200, 200 ) );
		do_line( display->pImage, x + MARK_SIZE
			, y - MARK_SIZE
			, x + MARK_SIZE
			, y + MARK_SIZE
			, Color( 200, 200, 200 ) );
		do_line( display->pImage, x + MARK_SIZE
			, y + MARK_SIZE
			, x - MARK_SIZE
			, y + MARK_SIZE
			, Color( 200, 200, 200 ) );
		do_line( display->pImage, x - MARK_SIZE
			, y + MARK_SIZE
			, x - MARK_SIZE
			, y - MARK_SIZE
			, Color( 200, 200, 200 ) );

	}
}


void CPROC TickUpdateActors( PTRSZVAL psv );
// MyControlID
// MyValidatedControlData( arena*, arena, pc )
class arena:public PSI_SimpleControl
{
	DISPLAY display;
	PLIST actors;
public:
	arena():PSI_SimpleControl( NULL, "Square Arena" )
	{
		actors = NULL;
		display.GridColor =  Color( 0, 128, 255 );
		display.Background = Color( 0,0,0 );
		display.GridXSets = 10;
		display.GridXUnits = 10;
		display.GridYSets = 10;
		display.GridYUnits = 10;
		display.scale = 1;
		display.xbias = 0;
		display.ybias = 0;
		display.zbias = 1;
		SetPoint( display.origin, VectorConst_0 );
		AddTimer( 100, TickUpdateActors, (PTRSZVAL)this );
	}
	void UpdateActors( void )
	{
		actor *a;
		INDEX idx;
		LIST_FORALL( actors, idx, actor *, a )
		{
			if( a == (actor*)1 )
				continue; // not a real entry.
			a->Tick();
		}
		SmudgeCommon( pc );
	}
	int OnDraw( PSI_CONTROL pc )
	{
		display.pImage = GetControlSurface( pc );
		display.xbias = display.pImage->width / 2;
		display.ybias = display.pImage->height / 2;
//		display.
		ClearImageTo( display.pImage, BASE_COLOR_BLACK );
		DrawDisplayGrid( &display );
		{
			char msg[256], len;
			len = sprintf( msg, "X: %5g Y: %5g "
							 , REAL_X((&display), display.x)
							 , REAL_Y((&display), display.y)
							  );

			PutString( display.pImage, 5, 29+12, BASE_COLOR_WHITE, Color(0,0,1), msg );
			//UpdateDisplayPortion( display->hVideo, 5, 29, len*8, 12 );
		}
		DrawActors( &display, actors );
		return 0;
	}
	int OnMouse( PSI_CONTROL pc, S_32 x, S_32 y, _32 b )
	{
		int delx = x - display.x
		 	, dely = y - display.y;
		_POINT o;
		_POINT del;
		o[0] = REAL_X( (&display), x );
		o[1] = REAL_Y( (&display), y );
		o[2] = 0;

		if( delx > 0 )
			display.delxaccum ++;
		else if( delx < 0 )
			display.delxaccum--;
		if( dely > 0 )
			display.delyaccum ++;
		else if( dely < 0 )
			display.delyaccum--;
		{
			char msg[256], len;
			len = sprintf( msg, "X: %5g Y: %5g  (%3d,%3d)"
							 , REAL_X((&display), x)
							 , REAL_Y((&display), y)
							 , delx, dely );

			PutString( display.pImage, 5, 29+12, BASE_COLOR_WHITE, Color(0,0,1), msg );
			//UpdateDisplayPortion( display->hVideo, 5, 29, len*8, 12 );
		}
		if( b & MK_LBUTTON )
		{
				display.origin[0] += REAL_SCALE( (&display), x - display.x );
				display.origin[1] += REAL_SCALE( (&display), y - display.y );
				SmudgeCommon( pc );
		}
		if( b & MK_MBUTTON )
		{
			static _32 last;
			if( !last || (last + 2000 ) < GetTickCount() )
			{
				GetTickCount();
				DebugDumpMem();
			}
		}
		if( b & MK_RBUTTON )
		{
			//add a fake thing....
			//AddLink( &actors, (POINTER)1 );
			//INDEX idx = FindLink( &actors, (POINTER)1 );
			//actor **ppActor = (actor**)GetLinkAddress( &actors, idx );
			// pass the fake thing for the actor to be able to clear;
			actor *a = new actor( &actors, REAL_X( (&display), x ), REAL_Y( (&display), y ) );
			//actor *a = (actor*)GetLink( &actors, idx );
			AddLink( &actors, a );
			
		}
		display.x = x;
		display.y = y;
		display.b = b;

		return 0;
	}
};


void CPROC TickUpdateActors( PTRSZVAL psv )
{
	((arena*)psv)->UpdateActors();
}



int APIENTRY WinMain( HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmdLine, int nCmdShow )
{
	arena *a = new arena();
	DisplayFrame( a->pc );

	while( 1 )
		WakeableSleep(1151);
	return 0;
}