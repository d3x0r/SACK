
#include "local.h"


RAGDOLL_NAMESPACE



void PopulateBulletGround( struct BulletInfo *_bullet )
{
	/* build the ground */
	 btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0,1,0),1);
	 //groundShape= new btConeShape( 1000, 200 );



	  btDefaultMotionState* groundMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));

	  btRigidBody::btRigidBodyConstructionInfo
                groundRigidBodyCI(0,groundMotionState,groundShape,btVector3(0,0,0));
	  //groundRigidBodyCI.m_restitution
        _bullet->groundRigidBody = new btRigidBody(groundRigidBodyCI);
		lprintf( "old was %g", _bullet->groundRigidBody->getRestitution()  );
		_bullet->groundRigidBody->setRestitution( 1.0f );
		_bullet->groundRigidBody->setFriction( 10.0f );

	
		//_bullet->groundRigidBody->
		_bullet->dynamicsWorld->addRigidBody(_bullet->groundRigidBody);

}



void CreateBody( void )
{
	body_part *prior;
	body_part *prior_left;
	body_part *prior_right;
	body_part *body = new body_part();

	body->shape = new btBoxShape( btVector3( 3, 1, 7 ) );
	body->name = "Left Foot";

		btDefaultMotionState* fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,1.1,0)));

		btScalar mass = 1; // kilo-grams
        btVector3 fallInertia(0,0,0);
        body->shape->calculateLocalInertia(mass,fallInertia);

		btRigidBody::btRigidBodyConstructionInfo *fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);
		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.01 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);


	prior_left =
		prior = body;
	body = new body_part();
	body->name = "Left Ankle";
	
	body->shape = new btBoxShape( btVector3( 3, 1, 3 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,4.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);

	btTypedConstraint *hinge;
	hinge = new btHingeConstraint( *prior->fallRigidBody, *body->fallRigidBody
		, btVector3( 0, 1.5, -2.5 )
		, btVector3( 0, -1.5, 0 ) 
		, btVector3( 1, 0, 0 )
		, btVector3( 1, 0, 0 )
		, false // use reference frame A ?
		);
	//hinge->setAngularOnly( true );
	l.bullet.dynamicsWorld->addConstraint( hinge, true );

	prior_left =
		prior = body;
	body = new body_part();
	body->name = "Left Calf";
	
	body->shape = new btBoxShape( btVector3( 3, 12, 3 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,12 + 6.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);

	hinge = new btHingeConstraint( *prior->fallRigidBody, *body->fallRigidBody
		, btVector3( 0, 1.5, 0 )
		, btVector3( 0, -12.5, 0 ) 
		, btVector3( 0, 0, 1 )
		, btVector3( 0, 0, 1 )
		, false // use reference frame A ?
		);
	l.bullet.dynamicsWorld->addConstraint( hinge, true );

	prior_left =
		prior = body;
	body = new body_part();
	body->name = "Left Thigh";
	
	body->shape = new btBoxShape( btVector3( 3, 15, 3 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,16 + 24 + 6.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);

	hinge = new btHingeConstraint( *prior->fallRigidBody, *body->fallRigidBody
		, btVector3( 0, 12.5, 0 )
		, btVector3( 0, -15.5, 0 ) 
		, btVector3( 1, 0, 0 )
		, btVector3( 1, 0, 0 )
		, false // use reference frame A ?
		);
	//hinge->setAngularOnly( true );
	l.bullet.dynamicsWorld->addConstraint( hinge, true );
	prior_left = body;
	


	prior_right =
		prior =
		body = new body_part();
	body->name = "Right Foot";

	body->shape = new btBoxShape( btVector3( 3, 1, 7 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(12,1.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);

	prior_right =
		prior = body;
	body = new body_part();
	body->name = "Right Ankle";
	
	body->shape = new btBoxShape( btVector3( 2, 0.5, 2 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(12,4.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);


	hinge = new btHingeConstraint( *prior->fallRigidBody, *body->fallRigidBody
		, btVector3( 0, 1.5, -2.5 )
		, btVector3( 0, -1.5, 0 ) 
		, btVector3( 1, 0, 0 )
		, btVector3( 1, 0, 0 )
		, false // use reference frame A ?
		);
	l.bullet.dynamicsWorld->addConstraint( hinge, true );

	prior_right =
		prior = body;
	body = new body_part();
	body->name = "Right Calf";
	
	body->shape = new btBoxShape( btVector3( 3, 12, 3 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(12,12 + 6.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);

	hinge = new btHingeConstraint( *prior->fallRigidBody, *body->fallRigidBody
		, btVector3( 0, 1.5, 0 )
		, btVector3( 0, -12.5, 0 ) 
		, btVector3( 0, 0, 1 )
		, btVector3( 0, 0, 1 )
		, false // use reference frame A ?
		);
	l.bullet.dynamicsWorld->addConstraint( hinge, true );


	prior_right =
		prior = body;
	body = new body_part();
	body->name = "Right Thigh";
	
	body->shape = new btBoxShape( btVector3( 3, 15, 3 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(12,16 + 24 + 6.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);

	hinge = new btHingeConstraint( *prior->fallRigidBody, *body->fallRigidBody
		, btVector3( 0, 12.5, 0 )
		, btVector3( 0, -15.5, 0 ) 
		, btVector3( 1, 0, 0 )
		, btVector3( 1, 0, 0 )
		, false // use reference frame A ?
		);
	//hinge->setAngularOnly( true );
	l.bullet.dynamicsWorld->addConstraint( hinge, true );
	prior_right = body;

	

	prior = body;
	body = new body_part();
	body->name = "Hips";
	
	body->shape = new btBoxShape( btVector3( 6, 2, 3 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(6,2 + 16 + 1 + 16 + 24 + 6.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);

	hinge = new btPoint2PointConstraint( *prior_left->fallRigidBody, *body->fallRigidBody
		, btVector3( 0, 15.5, 0 )
		, btVector3( -6.5, 0, 0 ) 
		);
	//hinge->setAngularOnly( true );
	l.bullet.dynamicsWorld->addConstraint( hinge, true );

	hinge = new btPoint2PointConstraint( *prior_right->fallRigidBody, *body->fallRigidBody
		, btVector3( 0, 15.5, 0 )
		, btVector3( 6.5, 0, 0 ) 
		);
	//hinge->setAngularOnly( true );
	l.bullet.dynamicsWorld->addConstraint( hinge, true );



	prior = body;
	body = new body_part();
	body->name = "abdomen";
	
	body->shape = new btBoxShape( btVector3( 5, 4, 2.5 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(6,2 + 16 + 1 + 16 + 24 + 6.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);


	hinge = new btPoint2PointConstraint( *prior->fallRigidBody, *body->fallRigidBody
		, btVector3( 0, 3, -3 )
		, btVector3( 0, -5, -3 ) 
		);
	l.bullet.dynamicsWorld->addConstraint( hinge, true );


	prior = body;
	body = new body_part();
	body->name = "Chest Tilt";
	
	body->shape = new btBoxShape( btVector3( 5, 3, 5 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(6,2 + 16 + 1 + 16 + 24 + 6.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);

	hinge = new btHingeConstraint( *prior->fallRigidBody, *body->fallRigidBody
		, btVector3( 0, 4.0, 0 )
		, btVector3( 0, -4.0, 0 ) 
		, btVector3( 0, 0, 1 )
		, btVector3( 0, 0, 1 )
		, false // use reference frame A ?
		);
	l.bullet.dynamicsWorld->addConstraint( hinge, true );

	

	prior = body;
	body = new body_part();
	body->name = "Chest";
	
	body->shape = new btBoxShape( btVector3( 9, 8, 4.5 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(6,2 + 16 + 1 + 16 + 24 + 6.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);

	hinge = new btHingeConstraint( *prior->fallRigidBody, *body->fallRigidBody
		, btVector3( 0, 4, -7 )
		, btVector3( 0, -5.0, -7 ) 
		, btVector3( 1, 0, 0 )
		, btVector3( 1, 0, 0 )
		, false // use reference frame A ?
		);
	l.bullet.dynamicsWorld->addConstraint( hinge, true );


	prior = body;

	body = new body_part();
	body->name = "Left Arm";
	
	body->shape = new btBoxShape( btVector3( 2, 11, 2 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(6,2 + 16 + 1 + 16 + 24 + 6.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);

	hinge = new btPoint2PointConstraint( *prior->fallRigidBody, *body->fallRigidBody
		, btVector3( -9, 7.5, 0 )
		, btVector3( 0, 11, 0 ) 
		);
	l.bullet.dynamicsWorld->addConstraint( hinge, true );
	prior_left = body;

	body = new body_part();
	body->name = "Left Lower Arm";
	
	body->shape = new btBoxShape( btVector3( 2, 11, 2 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(6,2 + 16 + 1 + 16 + 24 + 6.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);

	hinge = new btHingeConstraint( *prior_left->fallRigidBody, *body->fallRigidBody
		, btVector3( 0, -11, -0 )
		, btVector3( 0, -11, -0 ) 
		, btVector3( 1, 0, 0 )
		, btVector3( 1, 0, 0 )
		, false // use reference frame A ?
		);
	l.bullet.dynamicsWorld->addConstraint( hinge, true );



	body = new body_part();
	body->name = "Right Arm";
	
	body->shape = new btBoxShape( btVector3( 2, 11, 2 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(6,2 + 16 + 1 + 16 + 24 + 6.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);

	hinge = new btPoint2PointConstraint( *prior->fallRigidBody, *body->fallRigidBody
		, btVector3( 9, 7.5, 0 )
		, btVector3( 0, 11, 0 ) 
		);
	l.bullet.dynamicsWorld->addConstraint( hinge, true );

	prior_right = body;

	body = new body_part();
	body->name = "Right Lower Arm";
	
	body->shape = new btBoxShape( btVector3( 2, 11, 2 ) );

	fallMotionState =
                new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(6,2 + 16 + 1 + 16 + 24 + 6.1,0)));

		body->shape->calculateLocalInertia(mass,fallInertia);

		fallRigidBodyCI = new btRigidBody::btRigidBodyConstructionInfo(mass,fallMotionState,body->shape,fallInertia);

		//fallRigidBodyCI.m_startWorldTransform.setOrigin( btVector3( patch->origin[0], patch->origin[1], patch->origin[2] )  );
        body->fallRigidBody = new btRigidBody(*fallRigidBodyCI);
		body->fallRigidBody->setRestitution( 0.4 );
		body->fallRigidBody->setFriction( 0.8f ) ;

	l.bullet.dynamicsWorld->addRigidBody(body->fallRigidBody);

	hinge = new btHingeConstraint( *prior_right->fallRigidBody, *body->fallRigidBody
		, btVector3( 0, -11, -0 )
		, btVector3( 0, -11, -0 ) 
		, btVector3( 1, 0, 0 )
		, btVector3( 1, 0, 0 )
		, false // use reference frame A ?
		);
	l.bullet.dynamicsWorld->addConstraint( hinge, true );




}


PRELOAD( InitRagdoll )
{
	PopulateBulletGround( &l.bullet );

   CreateBody();

}


RAGDOLL_NAMESPACE_END


