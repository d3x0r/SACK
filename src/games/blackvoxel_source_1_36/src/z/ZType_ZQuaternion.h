#ifndef Z_ZTYPE_ZQUATERNION_H
#define Z_ZTYPE_ZQUATERNION_H

// #ifndef Z_ZTYPE_ZVECTOR3D_H
// #  include "ZType_ZVector3d.h"
// #endif

#ifndef Z_ZTYPES_H
#  include "ZTypes.h"
#endif


class ZQuaternion
{
public:
	double quaternion[4];
	
	inline ZQuaternion() { 
		quaternion[0] = 0;
		quaternion[1] = 0;
		quaternion[2] = 0;
		quaternion[3] = 0;
	}

	inline ZQuaternion( double yaw, double pitch, double roll )
	{
	}
#if 0
	void GetRotationMatrix( PTRANSFORM pt, RCOORD *quat )
{
//	t = Qxx+Qyy+Qzz (trace of Q)
//r = sqrt(1+t)
//w = 0.5*r
//x = copysign(0.5*sqrt(1+Qxx-Qyy-Qzz), Qzy-Qyz)
//y = copysign(0.5*sqrt(1-Qxx+Qyy-Qzz), Qxz-Qzx)
//z = copysign(0.5*sqrt(1-Qxx-Qyy+Qzz), Qyx-Qxy)
	/*
	where copysign(x,y) is x with the sign of y:
	copysign(x,y) = sign(y) |x|;

	*/
	/*
	t = Qxx+Qyy+Qzz
r = sqrt(1+t)
s = 0.5/r
w = 0.5*r
x = (Qzy-Qyz)*s
y = (Qxz-Qzx)*s
z = (Qyx-Qxy)*s
*/
	RCOORD t = pt->m[0][0] + pt->m[1][1] + pt->m[2][2];	RCOORD r = sqrt(1+t);	RCOORD s = ((RCOORD)0.5)/r;	quat[0] = ((RCOORD)0.5)*r;	quat[1] = (pt->m[2][1]-pt->m[1][2])*s;	quat[2] = (pt->m[0][2]-pt->m[2][0])*s;	quat[3] = (pt->m[1][0]-pt->m[0][1])*s;
}
#endif

};


#endif