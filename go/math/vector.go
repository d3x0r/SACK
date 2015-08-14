package math

import "math"

// change this type for double precision
type RealCoord float32

type Vector4 [4]RCoord
type Vector3 [3]RCoord
type Matrix3d [4]Vector4

type Transform struct {
	Matrix Matrix3d
        Scale Vector3
        
}

//func Add4( 




func GetPitch( m *Matrix ) RealCoord
{
	if( ( m[2][2] > 0.707 ) ) {
		return atan2( m[2][1], m[2][2] ) 
	} else if( ( m[2][2] < -0.707 ) ) {
		return atan2( m[2][1], -m[2][2] ) 
	} else if( m[2][0] < 0 ) {
	 	return atan2( m[2][1], -m[2][0] )
	}
	return atan2( m[2][1], m[2][0] )
}

func GetPitchDegrees( m *Matrix ) RealCoord {
	return GetPitch( m ) * 360.0 / 2.0*3.14159268;
}

func GetYaw( m *Matrix ) RealCoord
{
	if( m[2][1] < 0.707 && m[2][1] > -0.707 ) {
		return atan2( m[2][0], m[2][2] )
	} else if( m[1][2] < 0 ) {
	       	return atan2( m[1][0], m[1][2] )
	}
      	return atan2( m[1][0], m[1][2] )
}

func GetYawDegrees( m *Matrix ) RealCoord {
	return GetYaw( m ) * 360.0 / 2.0*3.14159268;
}

func GetRoll( m *Matrix ) RealCoord
{
	// sohcah toa  
	if( ( m[0][0] > 0.707 ) ) {
		return atan2( m[0][1], m[0][0] )
        } else if( ( m[0][0] < -0.707 ) ) {
		return atan2( m[0][1], -m[0][0] )
	} else if( m[0][2] < 0 ) {
       		return atan2( m[0][1], -m[0][2] )
       	}
        return atan2( m[0][1], m[0][2] )
}

func GetRollDegrees( m *Matrix ) RealCoord {
	return GetRoll( m ) * 360.0 / 2.0*3.14159268;
}

