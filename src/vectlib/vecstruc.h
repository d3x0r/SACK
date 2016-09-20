
VECTOR_NAMESPACE

struct motion_frame_tag
{
	// if rocket is 0, then it's virtual point camera mode.
	// that is motion is always forward, with rocket only
   // acceleration is applied forward, speed is constant to the world.
   int rocket;
   // speed right, up, forward ( use vRight, vUp, vForward to index array )
	RCOORD speed[3];
	// acceleration right, up and forward (after rotation matrix is applied, this is always relative
   // to the current transformation's forward, left and right.   ( use vRight, vUp, vForward to index array )
   RCOORD accel[3];
	// pitch, yaw, roll delta ( use vRight, vUp, vForward to index array )
	RCOORD rotation[3];
	// rot_accel is not used... just rotation velocity. ( use vRight, vUp, vForward to index array )
    // pitch, yaw, roll delta
	RCOORD rot_accel[3];
   // how long it takes to move one unit vector of speed
	RCOORD speed_time_interval;
   // how long it takes to rotate one unit vector of rotation
	RCOORD rotation_time_interval;
   // what time the last time we processed this matrix for Move.
	uint32_t last_tick;
   // rotation stepping for consistant rotation
	int nTime;
   // list of void(*)(uintptr_t,PTRANSFORM)
	PLIST callbacks;
   // actually uintptr_t storage...
	PLIST userdata; 

};

/* This structure maintains basically an inertial frame for an
   object. It contains the current orientation and position of
   an object. But it also contains speed and acceleration
   factors.                                                    */
struct transform_tag 
{
   // have to store partial values for rotation and
   // scale... since these terms may not be all together
   // specified... the remaining values in the matrix
   // can just be set - since they have no other dependant terms.

   // may have to keep matrix full symetry for multiplication
   // purposes... so that a sub-translation matrix may be modified
   // from a parent's...

   // requires [4][4] for use with opengl
  //   m[x][0] m[x][1] = partials... m[x][2] = multiplied value.
  // s*rcos[0][0]*rcos[0][1] sin sin   (0)
                   // sin s*rcos[1][0]*rcos[1][1] sin   (0)
                   // sin sin s*rcos[2][0]*rcos[2][0]   (0)
                   // tx  ty  tz                        (1)
//**************************************
//    AKA
//   MATRIX m;    // vRight   (n,n,n,n)
//                // vUp      (n,n,n,n)
//                // vForward (n,n,n,n)
//                // vIn      (n,n,n,n)
//*************************************
	MATRIX m;
	
   /* scalar, which can apply independant scalar values to
      resulting x y and y transformation. */
	RCOORD s[3];
   // Optional extension for transforms that track things that move, or move themselves.
   struct motion_frame_tag *motion;
};

VECTOR_NAMESPACE_END

