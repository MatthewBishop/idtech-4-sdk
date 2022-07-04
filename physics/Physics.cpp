

#pragma hdrstop
#include "../../idlib/precompiled.h"

#include "../Game_local.h"

ABSTRACT_DECLARATION( idClass, idPhysics )
END_CLASS


/*
================
idPhysics::~idPhysics
================
*/
idPhysics::~idPhysics() {
}

/*
================
idPhysics::Save
================
*/
void idPhysics::Save( idSaveGame *savefile ) const {
}

/*
================
idPhysics::Restore
================
*/
void idPhysics::Restore( idRestoreGame *savefile ) {
}

/*
================
idPhysics::SetClipBox
================
*/
void idPhysics::SetClipBox( const idBounds &bounds, float density ) {
	SetClipModel( new (TAG_PHYSICS_CLIP) idClipModel( idTraceModel( bounds ) ), density );
}

/*
================
idPhysics::SnapTimeToPhysicsFrame
================
*/
int idPhysics::SnapTimeToPhysicsFrame( int t ) {
	return MSEC_ALIGN_TO_FRAME( t );
}
