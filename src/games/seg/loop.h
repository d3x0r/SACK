
#include <configscript.h>


class Loop 
{
	int nPoints;
	VECTOR *pPoints;
	PLIST loops;

	VECTOR origin; // scalar transform
public:
	CDATA color;
	CDATA color2;
public:
	Loop()
	{
		color = BASE_COLOR_BLUE;
		pPoints = NULL;
		loops = NULL;
	}

	Loop( RCOORD outer, RCOORD top, RCOORD bottom, int segments, PCVECTOR origin, PCVECTOR up, CDATA color )
	{
		pPoints = NULL;
		loops = NULL;
		this->color = color;
		MakeVerticalRing( outer, top, bottom, segments, origin, up );
	}

	Loop( RCOORD inner, RCOORD outer, int segments, PCVECTOR origin, PCVECTOR up, CDATA color )
	{
		pPoints = NULL;
		loops = NULL;
		this->color = color;
		MakeFlatRing( inner, outer, segments, origin, up );
	}

	void SetColor( CDATA color )
	{
		this->color = color;
		
	}

	void SetOtherColor( CDATA color )
	{
		this->color2 = color;		
	}


	RCOORD BField( VECTOR result, PCVECTOR m_normal, RCOORD m_scalar, RCOORD r, RCOORD theta )
	{
		static PTRANSFORM pt;
		if( !pt )
			pt = CreateTransform();
		RotateAbs( pt, 0, 0, theta );
		Apply( pt, result, m_normal );
		// 1.2566370614 = 
		//The magnetic constant has the exact (defined)[1] value µ0 = 4?×10?7 H·m?1? 1.2566370614…×10?6 H·m?1 or N·A?2)
		scale( result, result, (1.2566370614/(4*(3.14159))) 
			* m_scalar * (1/(r*r*r)) 
			* sqrt(1+3*sin(3.14159/2 - theta)*sin(3.14159/2 - theta)) );
	}

	void MakeFluxLoop(  RCOORD r1, RCOORD r2
		, PCVECTOR origin1, PCVECTOR up1
		, PCVECTOR origin2, PCVECTOR up2
		, int segments
		, PCVECTOR origin
		, PCVECTOR up // ... it's the axis the loop is around
		)
	{
		SetPoint( this->origin, origin );
		if( pPoints )
			Release( pPoints );
		this->nPoints = segments;

		// this starts and ends on a triangle, so it doesn't need one less point
		pPoints = NewArray( VECTOR, 1 + segments * 2 );

		int max = segments*2;
		int phase;
		SetPoint( pPoints[0], origin1 );
		SetPoint( pPoints[max], origin2 );
		VECTOR norm_up1;
		VECTOR norm_up2;
		SetPoint( norm_up1, up1 );
		normalize( norm_up1 );
		SetPoint( norm_up2, up2 );
		normalize( norm_up2 );
		VECTOR Midway;
		VECTOR Midway1;
		crossproduct( Midway1, up1, up ); 
		VECTOR Midway2;
		crossproduct( Midway2, up2, up ); 
		normalize( Midway1 );
		normalize( Midway2 );
		add( Midway, Midway1, Midway2 );
		normalize( Midway );
		// 0-3.14159
		RCOORD delta_angle = acos( CosAngle( up1, up2 ) );
		PTRANSFORM t = CreateTransform();
		phase = 1;

		/*
		                 \    
		\      |           --     |
		  45			     45
		   --  N  --	      --  N  --
		    \     /		       \     /
			  \./ 		         \./ 
		90	  / \		   90	 / \
			/     \		       /     \
		   --  S  --	      --  S  --
          45			   -- 45
		/      |		 /        |

		*/

		/* B(m,r,lambda) = perm/4*pi * m/r^3 sqrt(1+3sin(lambda)*sin(lambda)


		/*
		for( ; phase < segments/4; phase++ )
		{
			ClearTransform( t );
			RotateAround( t, up, delta_angle/(segments/4) );

			//Translate( t, origin1 );
			VECTOR center;
			center[vRight] = r1;
			center[vForward] = 0;
			center[vUp] = 0;

			addscaled( pPoints[1 + phase * 2], center, up, width/2 );
			addscaled( pPoints[2 + phase * 2], center, up, -width/2 );
			pPoints[2 + phase * 2], 

		}
		*/
	}

	void MakeMobiusLoop( RCOORD radius, RCOORD width, int turns, int segments, PCVECTOR origin, PCVECTOR up, PCVECTOR right )
	{
		VECTOR top;
		VECTOR bottom;
		VECTOR center;

		center[vUp] = 0;
		center[vRight] = radius;
		center[vForward] = 0;



	}


	void MakeVerticalRing( RCOORD outer, RCOORD top, RCOORD bottom, int segments, PCVECTOR origin, PCVECTOR up )
	{
		SetPoint( this->origin, origin );
		if( pPoints )
			Release( pPoints );
		this->nPoints = segments;
		pPoints = NewArray( VECTOR, 2 + segments * 2 );

		PTRANSFORM tmp = CreateTransform();
		int n;
		VECTOR one;
		VECTOR norm_up;
		SetPoint( norm_up, up );
		normalize( norm_up );
		if( up[vUp] == 0 )
		{
			one[vRight] = -up[vForward];
			one[vForward] = up[vRight];
			one[vUp] = 0;
		}
		else
		{
			one[vRight] = -up[vUp];
			one[vForward] = 0;
			one[vUp] = up[vRight];
		}
		normalize( one );
		for( n = 0; n <= segments; n++ )
		{
			ClearTransform( tmp );
			RotateAround( tmp, norm_up, 2*3.14159 * n / segments );
			VECTOR normal;
			VECTOR on_plane;
			Apply( tmp, normal, one );
			scale( on_plane, normal, outer );

			addscaled( pPoints[n*2 + 0], on_plane, up, top );
			addscaled( pPoints[n*2 + 1], on_plane, up, bottom);
		}
		DestroyTransform( tmp );
	}

	void MakeFlatRing( RCOORD inner, RCOORD outer, int segments, PCVECTOR origin, PCVECTOR up )
	{
		SetPoint( this->origin, origin );
		if( pPoints )
			Release( pPoints );
		this->nPoints = segments;
		pPoints = NewArray( VECTOR, 2 + segments * 2 );

		PTRANSFORM tmp = CreateTransform();
		int n;
		VECTOR one;
		VECTOR norm_up;
		SetPoint( norm_up, up );
		normalize( norm_up );
		if( up[vUp] == 0 )
		{
		one[vRight] = -up[vForward];
		one[vForward] = up[vRight];
		one[vUp] = 0;
		}
		else
		{
			one[vRight] = -up[vUp];
			one[vForward] = 0;
			one[vUp] = up[vRight];
		}
		normalize( one );
		for( n = 0; n <= segments; n++ )
		{
			ClearTransform( tmp );
			RotateAround( tmp, norm_up, 2*3.14159 * n / segments );
			VECTOR normal;
			Apply( tmp, normal, one );
			
			addscaled( pPoints[n*2 + 0], _0, normal, inner );
			addscaled( pPoints[n*2 + 1], _0, normal, outer );
		}
		DestroyTransform( tmp );
	}

	void MakeProcess( void (*f)( PVECTOR output, arg_list *args ) )
	{
	}


	void MakeToneTowerBand( PCVECTOR o, PCVECTOR up, RCOORD outer, RCOORD inner, RCOORD low, RCOORD high, int segments )
	{
		SetPoint( this->origin, o );
		if( pPoints )
			Release( pPoints );
		this->nPoints = segments;
		pPoints = NewArray( VECTOR, 2 + segments * 2 );

		PTRANSFORM tmp = CreateTransform();
		int n;
		VECTOR one;
		VECTOR low_point;
		VECTOR high_point;
		VECTOR norm_up;
		SetPoint( norm_up, up );
		normalize( norm_up );
		if( up[vUp] == 0 )
		{
			one[vRight] = -up[vForward];
			one[vForward] = up[vRight];
			one[vUp] = 0;
		}
		else
		{
			one[vRight] = -up[vUp];
			one[vForward] = 0;
			one[vUp] = up[vRight];
		}
		normalize( one );
	    SetPoint( low_point, _0 );
		for( n = 0; n <= segments; n++ )
		{
			ClearTransform( tmp );
			RotateAround( tmp, norm_up, 2*3.14159 * n / segments );
			VECTOR normal;

			Apply( tmp, normal, one );
			low_point[vUp] = high;

			addscaled( pPoints[n*2 + 0], low_point, normal, inner );

			low_point[vUp] = low;
			addscaled( pPoints[n*2 + 1], low_point, normal, outer );
		}
		DestroyTransform( tmp );

	}

	void MakeToneTower( PCVECTOR o, PCVECTOR up
		, RCOORD scale
		, RCOORD start_phase, RCOORD end_phase
		, RCOORD low, RCOORD high
		, int segments_around, int segments_length )
	{
		int n;
		SetPoint( origin, o );
		for( n = 0; n < segments_length; n++ )
		{
			Loop *loop = new Loop();
			RCOORD current_low = low + ( n * ( high-low ) ) / segments_length;
			RCOORD current_high = low + ( ( n + 1 ) * ( high-low ) ) / segments_length;

			loop->MakeToneTowerBand( o, up
										  , scale * 1 / ( ( start_phase + ( n * (end_phase-start_phase) / segments_length ) ) * ( start_phase + ( n * (end_phase-start_phase) /segments_length ) ) )
										  , scale * 1 / ( ( start_phase + ( (n+1) * (end_phase-start_phase) / segments_length ) ) * ( start_phase + ( (n+1) * (end_phase-start_phase) / segments_length ) ) )
										  , current_low, current_high
										  , segments_around );
			loop->color = color;
			AddLink( &loops, loop );
		}
	}

	void MakeSpiral( RCOORD outer, RCOORD inner, RCOORD low, RCOORD high
						, RCOORD turns // from low to high
						, RCOORD twists // along length
						, RCOORD twist_phase // to build rope sides
						, PCVECTOR o, PCVECTOR up )
	{

	}



	void Draw( void )
	{
		VECTOR here;
		int n;
		{
			INDEX idx;
			Loop *loop;
			LIST_FORALL( loops, idx, Loop*, loop )
			{
				loop->Draw();
			}
		}
		glBegin( GL_TRIANGLE_STRIP );
		glColor4ubv( (GLubyte*)&color );
		for( n = 0; n <= nPoints; n++ )
		{
			add( here, origin, pPoints[n*2+0] );
			glVertex3fv( here );
			add( here, origin, pPoints[n*2+1] );
			glVertex3fv( here );
		}
		glEnd();

		glBegin( GL_LINES );
		glColor4ub( 255,255,255,255 );
		for( n = 0; n < nPoints; n++ )
		{
			add( here, origin, pPoints[n*2+0] );
			glVertex3fv( here );
			add( here, origin, pPoints[n*2+1] );
			glVertex3fv( here );

			add( here, origin, pPoints[n*2+0] );
			glVertex3fv( here );
			add( here, origin, pPoints[n*2+2] );
			glVertex3fv( here );

			add( here, origin, pPoints[n*2+1] );
			glVertex3fv( here );
			add( here, origin, pPoints[n*2+3] );
			glVertex3fv( here );
		}
		glEnd();
	}
};
