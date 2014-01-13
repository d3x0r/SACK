#define FIX_RELEASE_COM_COLLISION
#define MAKE_RCOORD_SINGLE
#define USE_IMAGE_INTERFACE l.pii
#define USE_IMAGE_3D_INTERFACE l.pi3i
#define USE_RENDER3D_INTERFACE l.pr3i

#define DEFINES_DEKWARE_INTERFACE
#define PLUGIN_MODULE

#include <plugin.h>
#include <vectlib.h>
#include <virtuality.h>
#include <render3d.h>
#include <image3d.h>


static struct local_virtuality_interface_tag
{
   INDEX extension;
   PIMAGE_INTERFACE pii;
   PIMAGE_3D_INTERFACE pi3i;
   PRENDER3D_INTERFACE pr3i;
   PTRANSFORM transform;
   PLIST objects;
} l;

struct virtuality_object
{
	VECTOR o;
	Image label;
};

static PTRSZVAL OnInit3d( WIDE( "Virtuality interface" ) )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	l.pr3i = GetRender3dInterface();
	l.pii = GetImageInterface();
	l.pi3i = GetImage3dInterface();
	l.transform = camera;
	{
		POBJECT root_object = MakeCube();
		SetObjectColor( root_object, BASE_COLOR_CYAN );
		SetRootObject( root_object );

		POBJECT floor_plane = CreateObject();
		PFACET floor_plane_facet = AddPlane( floor_plane, _0, _Y, 0 );
		floor_plane_facet->color = BASE_COLOR_BROWN;
		PutIn( root_object, floor_plane );
	}
	return 1;
}

static void OnFirstDraw3d( WIDE( "Terrain View" ) )( PTRSZVAL psvInit )
{
}

static LOGICAL OnUpdate3d( WIDE( "Terrain View" ) )( PTRANSFORM origin )
{
	l.transform = origin;
return TRUE;
}

static void DrawLabel( struct virtuality_object *vobj )
{
	//btTransform trans;
	//patch->fallRigidBody->getMotionState()->getWorldTransform( trans );
	//btVector3 r = trans.getOrigin();

	{
		PTRANSFORM t = GetImageTransformation( vobj->label );
		_POINT p;
		_POINT p2;
		p[0] = 0;
		p[1] = 10/*PLANET_RADIUS*/ * 1.4f;
		p[2] = 0;
		//ApplyRotation( l.transform, p2, p );
		Translate( t, vobj->o[0], vobj->o[1], vobj->o[2] );		
		Render3dImage( vobj->label, TRUE );
	}
}


static void OnDraw3d( WIDE( "Virtuality interface" ) )( PTRSZVAL psvInit )
{
	INDEX idx;
	struct virtuality_object *vobj;
	LIST_FORALL( l.objects, idx, struct virtuality_object *, vobj )
	{
		DrawLabel( vobj );
	}
   // virtuality doesn't have its own hooks...

}

static void UpdateLabel( struct virtuality_object *vobj, CTEXTSTR text )
{
	_32 w, h;
	GetStringSize( text, &w, &h );
	ResizeImage( vobj->label, w, h );
	SetImageTransformRelation( vobj->label, IMAGE_TRANSFORM_RELATIVE_CENTER, NULL );
	ClearImage( vobj->label );
	PutString( vobj->label, 0, 0, BASE_COLOR_WHITE, 0, text );
}
static int GetVector( VECTOR pos, PSENTIENT ps, PTEXT *parameters, LOGICAL log_error )
{
	PTEXT original = parameters?parameters[0]:NULL;
	S_64 iNumber;
	double fNumber;
	int bInt;
	PTEXT pResult;
	PTEXT temp;
	int n;
	for( n = 0; n < 3; n++ )
	{
		temp = GetParam( ps, parameters ); // does variable substitution etc...
		if( !temp )
		{
			if( log_error )
				S_MSG( ps, "vector part %d is missing", n + 1 );
			parameters[0] = original;
			return 0;
		}

		if( IsSegAnyNumber( &temp, &fNumber, &iNumber, &bInt ) )
		{
			if( bInt )
				pos[n] = (RCOORD)iNumber;
			else
				pos[n] = (RCOORD)fNumber;
		}
		else
		{
			if( log_error )
				S_MSG( ps, "vector part %d is not a number", n + 1 );
			parameters[0] = original;
			return 0;
		}
		log_error = 1;
	}
	return 1;
}


static int OnCreateObject( WIDE("Point Label"), WIDE( "This is a point in space that has text") )(PSENTIENT ps,PENTITY pe_created,PTEXT parameters)
{
	if( !l.extension )
		l.extension = RegisterExtension( WIDE( "Point Label" ) );
	{
		struct virtuality_object *vobj = New( struct virtuality_object );
		SetLink( &pe_created->pPlugin, l.extension, vobj );
	}
	{
		struct virtuality_object *vobj = (struct virtuality_object *)GetLink( &pe_created->pPlugin, l.extension );
		vobj->label = MakeImageFile( 0, 0 );
		UpdateLabel( vobj, "Label" );
		SetPoint( vobj->o, _0 );
		vobj->o[vForward] = 0;
		AddLink( &l.objects, vobj );

		{
			VECTOR pos;
			if( GetVector( pos, ps, &parameters, 0 ) )
				SetPoint( vobj->o, pos );
		}
	}
	return 0;
}

static PTEXT ObjectVolatileVariableSet( WIDE("Point Label"), WIDE("text"), WIDE( "Set label text") )(PENTITY pe, PTEXT value )
{
	struct virtuality_object *vobj = (struct virtuality_object *)GetLink( &pe->pPlugin, l.extension );
	PTEXT text = BuildLine( value );
	
	UpdateLabel( vobj, GetText( text ) );
	LineRelease( text );
   return NULL;
}

static PTEXT ObjectVolatileVariableGet( WIDE("Point Label"), WIDE("position"), WIDE( "get current position") )(PENTITY pe, PTEXT *prior )
{
	struct virtuality_object *vobj = (struct virtuality_object *)GetLink( &pe->pPlugin, l.extension );
	PVARTEXT pvt = VarTextCreate();
	PTEXT result;
	vtprintf( pvt, "%g %g %g", vobj->o[0], vobj->o[1], vobj->o[2] );
	result = VarTextGet( pvt );
	VarTextDestroy( &pvt );
	return result;
}

static PTEXT ObjectVolatileVariableSet( WIDE("Point Label"), WIDE("position"), WIDE( "get current position") )(PENTITY pe, PTEXT value )
{
	struct virtuality_object *vobj = (struct virtuality_object *)GetLink( &pe->pPlugin, l.extension );
	PTEXT line = BuildLine( value );
	float vals[3];
	if( sscanf( GetText( line ), "%g %g %g", vals+0, vals+1, vals+2 ) == 3 )
	{
		vobj->o[0] = (RCOORD)vals[0];
		vobj->o[1] = (RCOORD)vals[1];
		vobj->o[2] = (RCOORD)vals[2];
	}
	LineRelease( line );
   return NULL;
}

static int ObjectMethod( WIDE("Point Label"), WIDE("move"), WIDE( "Friendly description") )(PSENTIENT ps, PENTITY pe_object, PTEXT parameters)
{
	VECTOR pos;
	struct virtuality_object *vobj = (struct virtuality_object *)GetLink( &pe_object->pPlugin, l.extension );
	if( vobj )
	{
		if( GetVector( pos, ps, &parameters, 1 ) )
			SetPoint( vobj->o, pos );
	}
   return 0;
}
