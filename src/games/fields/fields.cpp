
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

#include "../include/virtuality/plane.h"
//#include "local.h"



struct {
   int nPoints;
	VECTOR *points;
} x;


static struct local_information {
	int frame;
   RCOORD time_scale;
   struct particle *particles;
   struct particle *last_particle;  // can initialize this to &particles and have a circular list
} l;

struct particle
{
	DeclareLink( struct particle );  // all particles are linked in this list. (
   int in_process;
	int frame;  // used for collision resolution.
   struct particle *checking;
	PLIST relavent_particles;

	RCOORD size;  // this might be extended someday/ but let's start spherical.  default 1.0

	RCOORD mass;  // default of 1.0
	RCOORD location[DIMENSIONS]; // where the mass is; affects other masses.

	RCOORD velocity[DIMENSIONS]; // normalize to get heading

   RCOORD up[DIMENSIONS];  // up (what spin is relative to when crossed with velocity)
   RCOORD spin; // a scalar of rotation around the normalized heading.  It is a rate of change.

	void (*gravity_falloff)( RCOORD *result, RCOORD distance );  // translate distance by a function, default 1/(d*d).. maybe it's just 1/d applied as acceleration which is t^2

	RCOORD charge; // default of 1.0
	void (*charge_falloff)( RCOORD *result, RCOORD distance );  // translate distance by a function, default 1/(d), applied as acceleration, so distance will square

   // location is shared between mass and charge

   RCOORD magnetism; // apply a scalar ... default of 1.0
	RCOORD north[DIMENSIONS];  // north (or south)  separate from velocity direction.  In a digital world it would be possible for north and another particles inverted north could be parallel, and no spin applied...
	void (*magnetic_torque )( RCOORD *result_spin, RCOORD *north1, RCOORD separation, RCOORD *north2 );

	void (*magnetic_falloff)( RCOORD *result, RCOORD distance );  // translate distance by a function default to 1/d
   void (*deflection)( RCOORD **result, RCOORD *velocity );
};

// all of these are results of a veleocity applied. these are the rates of change, not the end
//
struct dollardite
{
	// ratio of physical to metric dimension
   //
	RCOORD time;  // metrical
	RCOORD space;  // metrical

	// change in dielectric induction is current in I (amps)
	// PSI/t = I
   /// I can be a displacment curernt or a conduction current
	RCOORD dielctricity;  // physical
   RCOORD PSI; // couloumb
	// change in magnetic induction is voltage in E (volts)
	// PHI/t = E
	// E can be a kinetic force
   // E can be a static potential


	RCOORD magnetism;   // physical

	// Q/PSI = electric induction Couloumb
   // Q/PHI = electric induction weber

   // change in magnetic / change in dielectric is impedance in Ohms
	// PHI/PSI = Z in Ohms
   // E = IR ( R = E/I)

   // PHI *(cross product) PSI = Q (power) which is counterspace in per-cm

	// dielectric/magnetic is admittance Y in Seimens
   // Y = PSI/PHI

	// Q in Planks is a union of two inductions
	//  magnetic induction
   //  dielectric induction
	// a union is a multiplication.
	// much like the union of two masses in gravitation is a multiplication
   //


	// S 1 = length   S2 = area  S3 = volume
	// S-1 = span  S-2 = density S-3 = concentration
	//  per-cm       per area           per volume





};

struct delta_dollardite
{
	// ratio of physical to metric dimension
   //
	RCOORD time;  // metrical
	RCOORD space;  // metrical
	// change in dielectric induction is current in I (amps)
	// PSI/t = I
	RCOORD dielctricity;  // physical


	RCOORD magnetism;   // physical

	// Q in Planks is a union of two inductions
	//  magnetic induction
   //  dielectric induction
	// a union is a multiplication.
	// much like the union of two masses in gravitation is a multiplication
   //

}


static void NormalizeValues( RCOORD *a, RCOORD *b )
{
	RCOORD l = sqrt( (a[0] * a[0] ) + (b[0] + b[0] ) );
	// sin/cos(a/b)...
   // each scaled vector must be on the circle.
   (*a) = (*a) / l;
   (*b) = (*b) / l;

}

static LOGICAL ApplyProcess( struct particle *particle, struct particle *other_particle )
{
	RCOORD here_to_there[DIMENSIONS];
	RCOORD direction[DIMENSIONS];
	sub( here_to_there, other_particle->location, particle->location );
	distance = Length( here_to_there );
	normalize( here_to_there );

	if( distance < particle->size || distance < other_particle->size )
	{
		if( other_particle->frame == l.frame )
		{
			// this ran into that.
		}
		else
		{
         // have to try and process the other particle.
		}
		return FALSE;
	}
	else
	{
		RCOORD accel[DIMENSIONS];
		RCOORD tmp;
		particle->gravity_falloff( &tmp, distance );
		tmp = tmp * l.time_scale;

		add_scaled( accel, particle->velocity, here_to_there, particle->mass * other_particle->mass * tmp );

      // falloffs are all the same, so just use the same result for now
		//particle->charge_falloff( &tmp, distance );
		add_scaled( accel, particle->velocity, here_to_there
					 , -particle->charge * other_particle->charge * tmp ); // negative is away, if same sign, repel




		if( CosAngle( here_to_there, particle->north ) > 0.707 )
		{
		}
      // falloffs are all the same, so just use the same result for now
		//particle->charge_falloff( &tmp, distance );
		add_scaled( accel, particle->velocity, here_to_there
					 , particle->magnetism * other_particle->magnetism * tmp
					  * CosAngle( particle->north, other_particle->north ) );
		// at 180, north and north oppose, so -1 is away..

		RAY north_plane;
		RAY south_plane;
      VECTOR tmp_v;
		AddScaled( north_plane.o, particle->location, particle->north, particle->size / 2 );
      SetPoint( north_plane.n, particle->north );
		AddScaled( south_plane.o, particle->location, particle->north, -particle->size / 2 );
		SetPoint( south_plane.n, particle->north );
		Invert( south_plane.n );

      if( AbovePlane( other_particle->north, add( tmp_v, other_particle->location, other_particle->north ), south_plane.n



	}
}

static void ProcessRelavent( struct particle *particle )
{
	INDEX idx;
	struct particle *other_particle;
	LIST_FORALL( particle->relavent_particles, idx, struct particle *, other_particle )
	{
		INDEX idx2;
		struct particle *other_particle2;

      // skip this, we're computing to satisfy this one....
		if( other_particle->in_process )
         continue;
      particle->in_process = 1;
		ProcessRelavent( other_particle );

		if( !ApplyProcess( particle, other_particle ) )
		{
			DeleteLink( &particle->relavent_particles, other_particle );
         DeleteLink( &other_particlle->relavent_particles, particle );
		}
		particle->in_process = 0;
      particle->frame = l.frame;
	}
}



static void ProcessParticles( void )
{
	struct particle *particle;
	particle = l.particles;
	while( particle )
	{
		if( particle->frame == l.frame )
		{
			particle = NextThing( particle );
         continue;
		}
		ProcessRelavent( particle );

      // check near-relavent particles for new interactions
		{
			INDEX idx;
			struct particle *other_particle;
			LIST_FORALL( particle->relavent_particles, idx, struct particle *, other_particle )
			{
				other_particle->checking = particle;
			}

			LIST_FORALL( particle->relavent_particles, idx, struct particle *, other_particle )
			{
				INDEX idx2;
				struct particle *other_particle2;
				other_particle->checking = particle;

				LIST_FORALL( other_particle->relavent_particles, idx2, struct particle *, other_particle2 )
				{
					if( ( other_particle2 != particle )
						&& ( other_particle2->checking != particle ) )
						if( ApplyProcess( particle, other_particle2 ) )
						{
							AddLink( &particle->relavent_particles, other_particle2 );
							AddLink( &other_particle2->relavent_particles, particle );
						}
				}
			}
			LIST_FORALL( particle->relavent_particles, idx, struct particle *, other_particle )
			{
				other_particle->checking = NULL;
			}
		}
		particle->frame = l.frame;
      particle = NextThing( particle )
	}

   // one last pass, find island particles
   particle = l.particles;
	while( particle )
	{
		if( particle->frame == l.frame )
		{
			particle = NextThing( particle );
         continue;
		}
		{
			struct particle *any_particle;
         // check every other particle.
			for( any_particle = NextThing( particle ); any_particle; any_particle = NextThing( any_particle ) )
			{
				if( ApplyProcess( particle, any_particle ) )
				{
					INDEX idx2;
					struct particle *other_particle2;
					any_particle->checking = particle;

					LIST_FORALL( any_particle->relavent_particles, idx2, struct particle *, other_particle2 )
					{
						if( other_particle2->checking ) // already checked
                     continue;
                  other_particle2->checking = particle;
						if( ( other_particle2 != particle )
							&& ( other_particle2->checking != particle ) )
							if( ApplyProcess( particle, other_particle2 ) )
							{
								AddLink( &particle->relavent_particles, other_particle2 );
								AddLink( &other_particle2->relavent_particles, particle );
							}
					}

					AddLink( &particle->relavent_particles, any_particle );
					AddLink( &any_particle->relavent_particles, particle );
				}
			}
		}
	}
}

static void gravity_falloff( RCOORD *result, RCOORD distance )
{
   return 1/(distance*distance);
}


static void charge_falloff( RCOORD *result, RCOORD distance )
{
   return 1/(distance*distance);
}


static void magnetism_falloff( RCOORD *result, RCOORD distance )
{
   return 1/(distance*distance*distance);
}

struct particle *SummonParticle( void )
{
	struct particle *particle = New( struct particle );
	MemSet( particle, 0, sizeof( struct particle ) );
   particle->size = ONE;
	particle->mass = ONE;
	particle->charge = ONE;

   particle->gravity_falloff = gravity_falloff;
   particle->charge_falloff = charge_falloff;
   particle->magnetic_falloff = magnetism_falloff;

	LinkThing( &l.particles, particle );
   return
}


static void CreateUniverse( void )
{
   int c;
	int x, y;
	int z = 0;
	for( c = -1; c <= 1; c++ )
	{
		for( x = -10; x <= 10; x++ )
		{
			for( y = -10; y <= 10; y++ )
			{
				struct particle *particle = SummonParticle();
            particle->location[vUp] = x * 100 + c * 25;
            particle->location[vUp] = y * 100 + c * 25;
				particle->location[vUp] = z * 100;
            particle->charge = c * particle->charge;
			}
		}
	}
}

static void OnDraw3d( WIDE("Field Interaction 1") )( uintptr_t psvView )
{
	glBegin( GL_LINES );
   glEnd();


}

static void OnBeginDraw3d( WIDE( "Field Interaction 1" ) )( uintptr_t psv,PTRANSFORM camera )
{
}

static void OnFirstDraw3d( WIDE( "Field Interaction 1" ) )( uintptr_t psvInit )
{
	// and really if initshader fails, it sets up in local flags and 
	// states to make sure we just fall back to the old way.
	// so should load the classic image along with any new images.
	if (GLEW_OK != glewInit() )
	{
		// okay let's just init glew.
		return;
	}

	//InitPerspective();
	//InitShader();

}

static LOGICAL OnUpdate3D( WIDE("Field Interaction 1") )( uintptr_t psvInit, PTRANSFORM eye_transform )
{
   return TRUE;
}



static uintptr_t OnInit3d( WIDE( "Field Interaction 1" ) )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	// keep the camera as a 
	return (uintptr_t)camera;
}

