
#include "local.h"

void ExtendEntityWithBrain( PENTITY pe_created )
{
	struct virtuality_object *vobj = (struct virtuality_object *)GetLink( &pe_created->pPlugin, l.extension );
	// if a brain is added, going to need a command processor on the object also...
      UnlockAwareness( vobj->ps = CreateAwareness( pe_created ) );

		vobj->brain = new BRAIN();
		PBRAIN_STEM pbs = new BRAIN_STEM( "Object Motion" );
		vobj->brain->AddBrainStem( pbs );

		pbs->AddOutput( new value(&vobj->speed[vForward]), "Forward -Backwards" );
		pbs->AddOutput( new value(&vobj->speed[vRight]), "Right -Left" );
		pbs->AddOutput( new value(&vobj->speed[vUp]), "Up -Down" );

		pbs = new BRAIN_STEM( "Object Rotation" );
		vobj->brain->AddBrainStem( pbs );

		pbs->AddOutput( new value(&vobj->rotation_speed[vForward]), "around Forward axis" );
		pbs->AddOutput( new value(&vobj->rotation_speed[vRight]), "around Right axis" );
		pbs->AddOutput( new value(&vobj->rotation_speed[vUp]), "around Up axis" );

		vobj->brain_board = CreateBrainBoard( vobj->brain );

}
