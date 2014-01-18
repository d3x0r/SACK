#define MAIN_SOURCE
#include "local.h"

PRELOAD( Init )
{
	l.pr3i = GetRender3dInterface();
	l.pii = GetImageInterface();
	l.pi3i = GetImage3dInterface();

	l.root_object = Virtuality_MakeCube( 1000 );

}

static Image tmp ;
static PTRSZVAL OnInit3d( WIDE( "Virtuality interface" ) )( PMatrix projection, PTRANSFORM camera, RCOORD *identity_depth, RCOORD *aspect )
{
	l.transform = camera;
	{
		//Image 
		tmp = LoadImageFile( "%resources%/images/AN00236511_001_l.jpg" );
		SetImageTransformRelation( tmp, IMAGE_TRANSFORM_RELATIVE_CENTER, NULL );
		SetObjectColor( l.root_object, BASE_COLOR_BLUE );
		SetRootObject( l.root_object );

		POBJECT floor_plane = CreateObject();
		PFACET floor_plane_facet = AddPlane( floor_plane, _0, _Y, 0 );
		floor_plane_facet->color = BASE_COLOR_BROWN;
		floor_plane_facet->image = tmp;
		PutIn( floor_plane, l.root_object );

		PutOn( floor_plane = Virtuality_MakeCube( 10 ), l.root_object );
		floor_plane->color = BASE_COLOR_NICE_ORANGE;
	}
	return 1;
}

static void OnFirstDraw3d( WIDE( "Terrain View" ) )( PTRSZVAL psvInit )
{
}

static LOGICAL OnUpdate3d( WIDE( "Virtuality interface" ) )( PTRANSFORM origin )
{
	l.transform = origin;
	
	INDEX idx;
	struct virtuality_object *vobj;
	LIST_FORALL( l.objects, idx, struct virtuality_object *, vobj )
	{
		VECTOR tmp;
		scale( tmp, vobj->speed, 1/256.0 );
		SetSpeed( vobj->object->Ti, tmp );
		scale( tmp, vobj->rotation_speed, 1/256.0 );
		SetRotation( vobj->object->Ti, tmp );
	}
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
		PCVECTOR o = GetOrigin( vobj->object->Ti );
		Translate( t, o[0], o[1], o[2] );		
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
		MemSet( vobj, 0, sizeof( struct virtuality_object ) );
		SetLink( &pe_created->pPlugin, l.extension, vobj );
	}
	{
		struct virtuality_object *vobj = (struct virtuality_object *)GetLink( &pe_created->pPlugin, l.extension );
		vobj->label = MakeImageFile( 0, 0 );
		UpdateLabel( vobj, "Label" );

      ExtendEntityWithBrain( pe_created );

		PutIn( vobj->object = Virtuality_MakeCube( 10 ), l.root_object );
		CreateTransformMotionEx( vobj->object->Ti, 0 );
		vobj->object->color = BASE_COLOR_DARKBLUE;
		{
			VECTOR pos;
			if( GetVector( pos, ps, &parameters, 0 ) )
				TranslateV( vobj->object->Ti, pos );
		}

		// don't put in objects until it's finished...
		AddLink( &l.objects, vobj );
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
		PCVECTOR o = GetOrigin( vobj->object->Ti );
	vtprintf( pvt, "%g %g %g", o[0], o[1], o[2] );
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
		// float precision may mismatch, so pass as discrete values...
		Translate( vobj->object->Ti,vals[0],vals[1], vals[2] );
	}
	LineRelease( line );
	return 0;
}



static int ObjectMethod( WIDE("Point Label"), WIDE("move"), WIDE( "set the position of the object") )(PSENTIENT ps, PENTITY pe_object, PTEXT parameters)
{
	VECTOR pos;
	struct virtuality_object *vobj = (struct virtuality_object *)GetLink( &pe_object->pPlugin, l.extension );
	if( vobj )
	{
		if( GetVector( pos, ps, &parameters, 1 ) )
			TranslateV( vobj->object->Ti, pos );
	}
   return 0;
}
