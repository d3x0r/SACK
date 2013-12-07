
// Source mostly from

#define MAKE_RCOORD_SINGLE
#define NO_FILEOP_ALIAS
#include <stdhdrs.h>
#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define NEED_VECTLIB_COMPARE

// define local instance.
#define TERRAIN_MAIN_SOURCE  
#include <vectlib.h>
#include <render.h>
#include <render3d.h>

#ifndef SOMETHING_GOES_HERE
#include <GL/glew.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>

#include <virtuality.h>
//#include "local.h"


static struct local_info
{
	PLIST loops;
	PIMAGE_INTERFACE pii;
} l;

#include "loop.h"


static PTRSZVAL OnInit3d( "Seg Renderer" )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	l.pii = GetImageInterface();
	Loop *loop = new Loop();
	loop->MakeFlatRing( 9, 10, 32, _0, _Z );
	AddLink( &l.loops, loop );

	RCOORD height = 4;
	RCOORD R1_IN = 4;
	RCOORD R1_OUT = 7;
	RCOORD R2_IN = 10;
	RCOORD R2_OUT = 13;
	RCOORD R3_IN = 16;
	RCOORD R3_OUT = 19;
	RCOORD ROLLER_R = 1;
	VECTOR top;

	// 1/34
	// 4, 7, 10, 13
	SetPoint( top, _0 );
	top[vUp] = height;
	AddLink( &l.loops, new Loop( R1_IN, R1_IN+(R1_OUT-R1_IN)*13/34, 32, top, _Y, BASE_COLOR_DARKGREY ) );
	AddLink( &l.loops, new Loop( R1_IN+(R1_OUT-R1_IN)*13/34, R1_IN+(R1_OUT-R1_IN)*23/34, 32, top, _Y, BASE_COLOR_LIGHTGREY ) );
	AddLink( &l.loops, new Loop( R1_IN+(R1_OUT-R1_IN)*23/34, R1_IN+(R1_OUT-R1_IN)*30/34, 32, top, _Y, BASE_COLOR_ORANGE ) );
	AddLink( &l.loops, new Loop( R1_IN+(R1_OUT-R1_IN)*30/34, R1_IN+(R1_OUT-R1_IN)*34/34, 32, top, _Y, BASE_COLOR_RED ) );

	AddLink( &l.loops, new Loop( R2_IN, R2_IN+(R2_OUT-R2_IN)*13/34, 32, top, _Y, BASE_COLOR_DARKGREY ) );
	AddLink( &l.loops, new Loop( R2_IN+(R2_OUT-R2_IN)*13/34, R2_IN+(R2_OUT-R2_IN)*23/34, 32, top, _Y, BASE_COLOR_LIGHTGREY ) );
	AddLink( &l.loops, new Loop( R2_IN+(R2_OUT-R2_IN)*23/34, R2_IN+(R2_OUT-R2_IN)*30/34, 32, top, _Y, BASE_COLOR_ORANGE ) );
	AddLink( &l.loops, new Loop( R2_IN+(R2_OUT-R2_IN)*30/34, R2_IN+(R2_OUT-R2_IN)*34/34, 32, top, _Y, BASE_COLOR_RED ) );

	AddLink( &l.loops, new Loop( R3_IN, R3_IN+(R3_OUT-R3_IN)*13/34, 32, top, _Y, BASE_COLOR_DARKGREY ) );
	AddLink( &l.loops, new Loop( R3_IN+(R3_OUT-R3_IN)*13/34, R3_IN+(R3_OUT-R3_IN)*23/34, 32, top, _Y, BASE_COLOR_LIGHTGREY ) );
	AddLink( &l.loops, new Loop( R3_IN+(R3_OUT-R3_IN)*23/34, R3_IN+(R3_OUT-R3_IN)*30/34, 32, top, _Y, BASE_COLOR_ORANGE ) );
	AddLink( &l.loops, new Loop( R3_IN+(R3_OUT-R3_IN)*30/34, R3_IN+(R3_OUT-R3_IN)*34/34, 32, top, _Y, BASE_COLOR_RED ) );

	{
		PTRANSFORM t = CreateTransform();
		RotateAbs( t, 0, 2*3.14159 / 12, 0 );
		int r;
		int o = 0;
		VECTOR tmp[2];
		VECTOR offset;
			tmp[0][vRight] = R1_OUT + ROLLER_R;
			tmp[0][vUp] = 0;
			tmp[0][vForward] = 0;
		for( r = 0; r < 12; r++ )
		{
			Apply( t, tmp[1-o], tmp[o] );

			loop= new Loop();
			AddLink( &l.loops, loop );
			SetPoint( offset, tmp[1-o] );
			offset[vUp] = 4;
			loop->MakeFlatRing( ROLLER_R-(ROLLER_R*0.25), ROLLER_R, 16, offset, _Y );

			loop= new Loop();
			AddLink( &l.loops, loop );
			offset[vUp] = -4;
			loop->MakeFlatRing( ROLLER_R-(ROLLER_R*0.25), ROLLER_R, 16, offset, _Y );

			loop= new Loop();
			AddLink( &l.loops, loop );
			loop->MakeVerticalRing( ROLLER_R, height, -height, 16, tmp[1-o], _Y );

			o = 1-o;

		}

		o = 0;
		RotateAbs( t, 0, 2*3.14159 / 22, 0 );
			tmp[0][vRight] = R2_OUT + ROLLER_R;
			tmp[0][vUp] = 0;
			tmp[0][vForward] = 0;
		for( r = 0; r < 22; r++ )
		{
			Apply( t, tmp[1-o], tmp[o] );

			loop= new Loop();
			AddLink( &l.loops, loop );
			SetPoint( offset, tmp[1-o] );
			offset[vUp] = 4;
			loop->MakeFlatRing( ROLLER_R-ROLLER_R/4, ROLLER_R, 16, offset, _Y );

			loop= new Loop();
			AddLink( &l.loops, loop );
			offset[vUp] = -4;
			loop->MakeFlatRing( ROLLER_R-ROLLER_R/4, ROLLER_R, 16, offset, _Y );

			loop= new Loop();
			AddLink( &l.loops, loop );
			loop->MakeVerticalRing( ROLLER_R, height, -height, 16, tmp[1-o], _Y );

			o = 1-o;

		}

			o = 0;
			tmp[0][vRight] = R3_OUT+ROLLER_R;
			tmp[0][vUp] = 0;
			tmp[0][vForward] = 0;
		RotateAbs( t, 0, 2*3.14159 / 32, 0 );
		for( r = 0; r < 32; r++ )
		{
			Apply( t, tmp[1-o], tmp[o] );

			loop= new Loop();
			AddLink( &l.loops, loop );
			SetPoint( offset, tmp[1-o] );
			offset[vUp] = 4;
			loop->MakeFlatRing( ROLLER_R-ROLLER_R/4, ROLLER_R, 16, offset, _Y );

			loop= new Loop();
			AddLink( &l.loops, loop );
			offset[vUp] = -4;
			loop->MakeFlatRing( ROLLER_R-ROLLER_R/4, ROLLER_R, 16, offset, _Y );

			loop= new Loop();
			AddLink( &l.loops, loop );
			loop->MakeVerticalRing( ROLLER_R, height, -height, 16, tmp[1-o], _Y );

			o = 1-o;

		}

		DestroyTransform( t );
	}

	{
		VECTOR v;
		v[vRight] = 1;
		v[vUp] = 5;
		v[vForward] = -2;
		loop = new Loop();
		loop->MakeFlatRing( 9, 10, 32, _0, v );
		AddLink( &l.loops, loop );
	}
	return 1;
}

static void OnFirstDraw3d( "Seg Renderer" )( PTRSZVAL psvInit )
{
}

static void OnBeginDraw3d( "Seg Renderer" )( PTRSZVAL psvInit, PTRANSFORM camera )
{

}

void OnDraw3d( "Seg Renderer" )( PTRSZVAL psvInit )
{
	INDEX idx;
	Loop *loop;
	LIST_FORALL( l.loops, idx, Loop*, loop )
	{
		loop->Draw();
	}
}

static LOGICAL OnMouse3d( "Virtuality" )( PTRSZVAL psvInit, PRAY mouse, _32 b )
{
	return 0;
}