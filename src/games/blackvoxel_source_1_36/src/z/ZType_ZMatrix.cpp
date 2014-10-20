


// #ifndef Z_ZTYPE_ZVECTOR3D_H
// #  include "ZType_ZVector3d.h"
// #endif

#ifndef Z_ZTYPES_H
#  include "ZTypes.h"
#endif

	double ZMatrix::pitch( void )
	{
		if( ( m[2][2] > 0.707 ) )
			return atan2( m[2][1], m[2][2] ) * (360 / (2*3.14159)); 
		else if( ( m[2][2] < -0.707 ) )
			return atan2( m[2][1], -m[2][2] ) * (360 / (2*3.14159)); 
		else
			if( m[2][0] < 0 )
				return atan2( m[2][1], -m[2][0] ) * (360 / (2*3.14159)); 
			else
				return atan2( m[2][1], m[2][0] ) * (360 / (2*3.14159)); 


		return atan2( -m[2][1], sqrt( 1-m[2][1] * m[2][1]) ) * (360 / (2*3.14159)); 
		// sohcah toa  
		return asin( m[0][2] ) * (360 / (2*3.14159));
	}
	double ZMatrix::yaw( void )
	{
		if( m[2][1] < 0.707 && m[2][1] > -0.707 )
		{
			return atan2( m[2][0], m[2][2] ) * (360 / (2*3.14159)); 
		}
		else
		{
			if( m[1][2] < 0 )
				return atan2( m[1][0], m[1][2] ) * (360 / (2*3.14159)); 
			else
				return atan2( m[1][0], m[1][2] ) * (360 / (2*3.14159)); 
		}
		// sohcah toa  
		//return atan2( m[0][1], m[0][0] ) * (360 / (2*3.14159)); 
		return asin( m[0][2] ) * (360 / (2*3.14159));
	}
	double ZMatrix::roll( void )
	{
		// sohcah toa  
		if( ( m[0][0] > 0.707 ) )
			return atan2( m[0][1], m[0][0] ) * (360 / (2*3.14159)); 
		else if( ( m[0][0] < -0.707 ) )
			return atan2( m[0][1], -m[0][0] ) * (360 / (2*3.14159)); 
		else
			if( m[0][2] < 0 )
				return atan2( m[0][1], -m[0][2] ) * (360 / (2*3.14159)); 
			else
				return atan2( m[0][1], m[0][2] ) * (360 / (2*3.14159)); 

		return asin( m[0][1] ) * (360 / (2*3.14159));
	}

