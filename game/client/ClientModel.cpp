//----------------------------------------------------------------
// ClientModel.cpp
//
// A non-interactive client-side model
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "ClientModel.h"

/*
===============================================================================

rvClientModel

===============================================================================
*/
CLASS_DECLARATION( rvClientEntity, rvClientModel )
END_CLASS


/*
================
rvClientModel::rvClientModel
================
*/
rvClientModel::rvClientModel ( void ) {
	memset ( &renderEntity, 0, sizeof(renderEntity) );
	worldAxis = mat3_identity;
	entityDefHandle = -1;
}

/*
================
rvClientModel::~rvClientModel
================
*/
rvClientModel::~rvClientModel ( void ) {
	FreeEntityDef ( );
}

/*
================
rvClientModel::FreeEntityDef
================
*/
void rvClientModel::FreeEntityDef ( void ) {
	if ( entityDefHandle >= 0 ) {
		gameRenderWorld->FreeEntityDef ( entityDefHandle );
		entityDefHandle = -1;
	}	
}

/*
================
rvClientModel::Spawn
================
*/
void rvClientModel::Spawn ( const idDict* spawnArgs ) {
	assert ( spawnArgs );

	spawnArgs->GetString ( "classname", "", classname );

	// parse static models the same way the editor display does
	gameEdit->ParseSpawnArgsToRenderEntity( spawnArgs, &renderEntity );
}

/*
================
rvClientModel::Think
================
*/
void rvClientModel::Think ( void ) {
	if( bindMaster && (bindMaster->GetRenderEntity()->hModel && bindMaster->GetModelDefHandle() == -1) ) {
		return;
	}
	UpdateBind();
	Present();
}

/*
================
rvClientModel::Present
================
*/
void rvClientModel::Present(void) {
	// Hide client entities bound to a hidden entity
	if ( bindMaster && (bindMaster->IsHidden ( ) || (bindMaster->GetRenderEntity()->hModel && bindMaster->GetModelDefHandle() == -1) ) ) {
		return;
	}

	renderEntity.origin = worldOrigin;
	renderEntity.axis = worldAxis;

	// add to refresh list
	if ( entityDefHandle == -1 ) {
		entityDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	} else {
		gameRenderWorld->UpdateEntityDef( entityDefHandle, &renderEntity );
	}		
}

/*
================
rvClientModel::SetCustomShader
================
*/
bool rvClientModel::SetCustomShader ( const char* shaderName ) {
	if ( shaderName == NULL ) {
		return false;
	}
	
	const idMaterial* material = declManager->FindMaterial( shaderName );

	if ( material == NULL ) {
		return false;
	}
	
	renderEntity.customShader = material;

	return true;
}

/*
================
rvClientModel::Save
================
*/
void rvClientModel::Save( idSaveGame *savefile ) const {
	savefile->WriteRenderEntity( renderEntity );
	savefile->WriteInt( entityDefHandle );

	savefile->WriteString ( classname );	// cnicholson: Added unsaved var

}

/*
================
rvClientModel::Restore
================
*/
void rvClientModel::Restore( idRestoreGame *savefile ) {
	savefile->ReadRenderEntity( renderEntity, NULL );
	savefile->ReadInt( entityDefHandle );

	savefile->ReadString ( classname );		// cnicholson: Added unrestored var

	// restore must retrieve entityDefHandle from the renderer
 	if ( entityDefHandle != -1 ) {
 		entityDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
 	}
}

