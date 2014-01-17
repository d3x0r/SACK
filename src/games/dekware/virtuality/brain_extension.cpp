
#include "local.h"

void ExtendEntityWithBrain( PENTITY pe_created )
{
	struct virtuality_object *vobj = (struct virtuality_object *)GetLink( &pe_created->pPlugin, l.extension );
	// if a brain is added, going to need a command processor on the object also...
      UnlockAwareness( vobj->ps = CreateAwareness( pe_created ) );

		vobj->brain = new BRAIN();
		PBRAIN_STEM pbs = new BRAIN_STEM( WIDE("Object Motion") );
		vobj->brain->AddBrainStem( pbs );

		pbs->AddOutput( new value(&vobj->speed[vForward]), WIDE("Forward -Backwards") );
		pbs->AddOutput( new value(&vobj->speed[vRight]), WIDE("Right -Left") );
		pbs->AddOutput( new value(&vobj->speed[vUp]), WIDE("Up -Down") );

		pbs = new BRAIN_STEM( WIDE("Object Rotation") );
		vobj->brain->AddBrainStem( pbs );

		pbs->AddOutput( new value(&vobj->rotation_speed[vForward]), WIDE("around Forward axis") );
		pbs->AddOutput( new value(&vobj->rotation_speed[vRight]), WIDE("around Right axis") );
		pbs->AddOutput( new value(&vobj->rotation_speed[vUp]), WIDE("around Up axis") );

		vobj->brain_board = CreateBrainBoard( vobj->brain );

}
