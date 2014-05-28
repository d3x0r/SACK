
// Source mostly from

#define MAKE_RCOORD_SINGLE
#define NO_FILEOP_ALIAS
#include <stdhdrs.h>
#define USE_RENDER_INTERFACE l.pri
#define USE_IMAGE_INTERFACE l.pii
#define USE_RENDER_3D_INTERFACE l.pr3i
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

#include "Sliders.h"

static struct local_info
{
	PLIST loops;
	PIMAGE_INTERFACE pii;
	PRENDER3D_INTERFACE pr3i;
	SliderFrame *sliders;
} l;

// loop requires l.pii
#include "loop.h"

static PTRSZVAL OnInit3d( "Seg Renderer" )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	l.pii = GetImageInterface();
	l.pr3i = GetRender3dInterface();

	Loop *loop;
	l.sliders = new SliderFrame();

	//loop = new Loop();
	//loop->MakeFlatRing( 9, 10, 32, _0, _Z );
	//AddLink( &l.loops, loop );

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

	if( 0 )
	{
		VECTOR v;
		SetPoint( v, _Y );
		Invert( v );

		//loop->SetColor( MakeAlphaColor( 0, 5, 0, 64 ) );
		loop = new Loop();
		loop->MakeToneTower( top, v, 15, 0.612, 1.612 //2, 12 //0.5, 3
			, 0, -12  // low-high
			, 16, 16 ); // segment counts
		AddLink( &l.loops, loop );
	}

	
	{
		int n;
		PTRANSFORM t = CreateTransform();
		int pt = 0;
			VECTOR v[2];
			VECTOR v1;
			VECTOR o[2];
			v[pt][vRight] = 1;
			v[pt][vUp] = 5;
			v[pt][vForward] = -2;
			o[pt][vRight] = 1.0;
			o[pt][vUp] = 0;
			o[pt][vForward] = 0;
		RotateAbs( t, 0, 3.14159 / 6, 0 );
		for( n = 0; n < 12; n++ )
		{
			Apply( t, o[1-pt], o[pt] );						
			Apply( t, v[1-pt], v[pt] );
			pt = 1 - pt;
			loop = new Loop();
			loop->MakeFlatRing( 9, 10, 32, o[pt], v[pt] );
			AddLink( &l.loops, loop );
		}
		for( n = 0; n < 12; n++ )
		{
			Apply( t, o[1-pt], o[pt] );						
			Apply( t, v[1-pt], v[pt] );
			pt = 1 - pt;

			loop = new Loop();
			loop->color = BASE_COLOR_RED;
			loop->MakeVerticalRing( 9, 0.5, -0.5, 32, o[pt], v[pt] );
			AddLink( &l.loops, loop );
		}
	}

/*
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
	*/
	return 1;
}

static void OnFirstDraw3d( "Seg Renderer" )( PTRSZVAL psvInit )
{
}

static void OnBeginDraw3d( "Seg Renderer" )( PTRSZVAL psvInit, PTRANSFORM camera )
{

}

static LOGICAL OnUpdate3d( "Seg Renderer" )( PTRANSFORM origin )
{
	PTRANSFORM pt_sliders = GetRenderTransform( GetFrameRenderer( l.sliders->frame ) );
	VECTOR v;
	VECTOR v2;
	v[vForward] = 10;
	v[vRight] = 50;
	v[vUp] = -20;
	GetOriginV( origin, v );
	ApplyInverse( origin, v2, v );
	ApplyRotationT( origin, pt_sliders, (PTRANSFORM)_I );
	TranslateV( pt_sliders, v2 );
	Invert( (P_POINT)GetAxis( pt_sliders, vUp ) );
	MoveRight( pt_sliders, 250 );
	MoveUp( pt_sliders, -250 );

	//MoveForward( pt_sliders, 10 );
	//MoveRight( pt_sliders, 50 );

	{
		static struct anim_state {
			int t;
			int target;
			int span;
			int mode; // 0 == expand origin
		} a;
		static int frame;
		static int start_time;
		int n;

		if( !start_time )
		{
			start_time = timeGetTime();

		}
		frame++;
		if( frame % 100 == 0 )
		{
			lprintf( "fps = %g", (float)frame / ((float)timeGetTime() - (float)start_time) );
		}
		if( !a.t )
		{
			a.t = timeGetTime();
			a.mode = 0;
			a.span = 2000;
			a.target = a.t + a.span;
		}
		else
		{
			a.t = timeGetTime();
			switch( a.mode )
			{
			case 0:
				if( a.t > a.target )
				{
					a.mode = 0;
					a.span = 3000;
					a.target = a.t + a.span;
				}
				break;
			}
		}

		PTRANSFORM t = CreateTransform();
		int pt = 0;
			VECTOR v[2];
			VECTOR v1;
			VECTOR o[2];
			v[pt][vRight] = (l.sliders->values[0] - 128) / 50.0;
			v[pt][vUp] = (l.sliders->values[1] - 128) / 50.0;
			v[pt][vForward] = (l.sliders->values[2] - 128) / 50.0;
			normalize( v[pt] );

			//if( a.mode == 0 )
			{
				o[pt][vRight] = (l.sliders->values[3] - 128) / 8.0; //11.0 - ( 20.0 * (a.target-a.t) / a.span );
				o[pt][vUp] = 0;
				o[pt][vForward] = 0;
			}
			if( 0 )
			{
				o[pt][vRight] = 1.0;
				o[pt][vUp] = 0;
				o[pt][vForward] = 0;
			}
		RotateAbs( t, 0, 3.14159 / 6, 0 );
		Loop *loop;
		LIST_FORALL( l.loops, n, Loop *, loop )
		{
			Apply( t, o[1-pt], o[pt] );						
			Apply( t, v[1-pt], v[pt] );
			pt = 1 - pt;
			if( n >= 12 )
				loop->MakeVerticalRing( 9, 0.5, -0.5, 32, o[pt], v[pt] );
			else
				loop->MakeFlatRing( 9, 10, 32, o[pt], v[pt] );
		}
		DestroyTransform( t );
	}
	return TRUE;
}

static void OnDraw3d( "Seg Renderer" )( PTRSZVAL psvInit )
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