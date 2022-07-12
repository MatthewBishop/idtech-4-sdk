//----------------------------------------------------------------
// ClientEntity.cpp
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"

ABSTRACT_DECLARATION( idClass, rvClientEntity )
END_CLASS

/*
================
rvClientEntity::rvClientEntity
================
*/
rvClientEntity::rvClientEntity( void ) {
	bindMaster = NULL;
	entityNumber = -1;

	bindOrigin.Zero();
	bindAxis.Identity();
	bindJoint = INVALID_JOINT;
	
	spawnNode.SetOwner( this );
	bindNode.SetOwner( this );

	memset( &refSound, 0, sizeof(refSound) );
	refSound.referenceSoundHandle = -1;

	gameLocal.RegisterClientEntity( this );
}

/*
================
rvClientEntity::~rvClientEntity
================
*/
rvClientEntity::~rvClientEntity( void ) {
	Unbind();
	gameLocal.UnregisterClientEntity( this );

	// Free sound emitter
	soundSystem->FreeSoundEmitter( SOUNDWORLD_GAME, refSound.referenceSoundHandle, true );
	refSound.referenceSoundHandle = -1;
}

/*
================
rvClientEntity::Present
================
*/
void rvClientEntity::Present ( void ) {
}

/*
================
rvClientEntity::FreeEntityDef
================
*/
void rvClientEntity::FreeEntityDef ( void ) {
}

/*
================
rvClientEntity::Think
================
*/
void rvClientEntity::Think ( void ) {
	UpdateBind ( );
	UpdateSound ( );
	Present ( );
}

/*
================
rvClientEntity::Bind
================
*/
void rvClientEntity::Bind ( idEntity* master, jointHandle_t joint ) {
	Unbind ( );

	if ( joint != INVALID_JOINT && !dynamic_cast<idAnimatedEntity*>(master) ) {
		gameLocal.Warning( "rvClientEntity::Bind: entity '%s' cannot support skeletal models.", master->GetName() );
		joint = INVALID_JOINT;
	}

	bindMaster = master;
	bindJoint  = joint;
	bindOrigin = worldOrigin;
	bindAxis   = worldAxis;

	bindNode.AddToEnd ( bindMaster->clientEntities );
	
	UpdateBind();
}

/*
================
rvClientEntity::Bind
================
*/
void rvClientEntity::Unbind	( void ) {
	if ( !bindMaster ) {
		return;
	}

	bindMaster = NULL;
	bindNode.Remove ( );
}

/*
================
rvClientEntity::SetOrigin
================
*/
void rvClientEntity::SetOrigin ( const idVec3& origin ) {
	if ( bindMaster ) {
		bindOrigin = origin;
	} else {
		worldOrigin = origin;
	}
}

/*
================
rvClientEntity::SetAxis
================
*/
void rvClientEntity::SetAxis ( const idMat3& axis ) {
	if ( bindMaster ) {
		bindAxis = axis * bindMaster->GetRenderEntity()->axis.Transpose();		
	} else {
		worldAxis = axis;
	}
}

/*
================
rvClientEntity::UpdateBind
================
*/
void rvClientEntity::UpdateBind ( void ) {
	if ( !bindMaster ) {
		return;
	}

	if ( bindJoint != INVALID_JOINT ) {
		static_cast<idAnimatedEntity*>(bindMaster.GetEntity())->GetJointWorldTransform ( bindJoint, gameLocal.time, worldOrigin, worldAxis );
	} else {
		bindMaster->GetPosition( worldOrigin, worldAxis );
		//if ( !bindMaster->GetPhysicsToVisualTransform( worldOrigin, worldAxis ) ) {
		//	bindMaster->GetPosition( worldOrigin, worldAxis );
		//}
	}

	worldOrigin += (bindOrigin * worldAxis);
	worldAxis    = bindAxis * worldAxis;
}

/*
================
rvClientEntity::IsClient
================
*/
bool rvClientEntity::IsClient ( void ) const {
	return true;
}

/*
================
rvClientEntity::DrawDebugInfo
================
*/
void rvClientEntity::DrawDebugInfo ( void ) const {
	idBounds bounds ( idVec3(-8,-8,-8), idVec3(8,8,8) );
	
	gameRenderWorld->DebugBounds ( colorGreen, bounds, worldOrigin );

	if ( gameLocal.GetLocalPlayer() ) {	
		gameRenderWorld->DrawText ( GetClassname ( ), worldOrigin, 0.1f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1 );
	}
}

/*
================
rvClientEntity::UpdateSound
================
*/
void rvClientEntity::UpdateSound( void ) {
	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if ( emitter ) {
		refSound.origin = worldOrigin;
		refSound.velocity = vec3_origin;
		emitter->UpdateEmitter( refSound.origin, refSound.velocity, refSound.listenerId, &refSound.parms );
	}
}

/*
================
rvClientEntity::StartSoundShader
================
*/
int rvClientEntity::StartSoundShader ( const idSoundShader* shader, const s_channelType channel, int soundShaderFlags )  {
	if ( !shader ) {
		return 0;
	}
	
	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if ( !emitter ) {
		refSound.referenceSoundHandle = soundSystem->AllocSoundEmitter( SOUNDWORLD_GAME );
	}

	UpdateSound();
	
	emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if( !emitter ) { 
		return( 0 );
	}

	emitter->UpdateEmitter( refSound.origin, refSound.velocity, refSound.listenerId, &refSound.parms );
	return emitter->StartSound( shader, channel, gameLocal.random.RandomFloat(), soundShaderFlags  );
}

/*
================
rvClientEntity::Size

Returns Returns memory size of an rvClientEntity.
================
*/

size_t rvClientEntity::Size ( void ) const {
	return sizeof( rvClientEntity );
}

/*
================
rvClientEntity::Save
================
*/
void rvClientEntity::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( entityNumber );

	// idLinkList<rvClientEntity>	spawnNode;		- reconstructed in the master entity load
	// idLinkList<rvClientEntity>	bindNode;		- reconstructed in the master entity load

	savefile->WriteVec3( worldOrigin );
	savefile->WriteMat3( worldAxis );

	bindMaster.Save( savefile );
	savefile->WriteVec3( bindOrigin );
	savefile->WriteMat3( bindAxis );
	savefile->WriteJoint( bindJoint );
	
	savefile->WriteRefSound( refSound );
}

/*
================
rvClientEntity::Restore
================
*/
void rvClientEntity::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( entityNumber );

	// idLinkList<rvClientEntity>	spawnNode;		- reconstructed in the master entity load
	// idLinkList<rvClientEntity>	bindNode;		- reconstructed in the master entity load

	savefile->ReadVec3( worldOrigin );
	savefile->ReadMat3( worldAxis );

	bindMaster.Restore( savefile );
	savefile->ReadVec3( bindOrigin );
	savefile->ReadMat3( bindAxis );
	savefile->ReadJoint( bindJoint );
	
	savefile->ReadRefSound( refSound );
}

/*
================
rvClientEntity::RunPhysics
================
*/
void rvClientEntity::RunPhysics ( void ) {
	idPhysics* physics = GetPhysics( );
	if( !physics ) {
		return;
	}

	idEntity* clientPhysics = gameLocal.entities[ENTITYNUM_CLIENT];
	static_cast<rvClientPhysics*>( clientPhysics )->currentEntityNumber = entityNumber;

	clientPhysics->SetPhysics( physics );
	physics->Evaluate ( gameLocal.time - gameLocal.previousTime, gameLocal.time );
	clientPhysics->SetPhysics( NULL );

	worldOrigin = physics->GetOrigin ( );
	worldAxis = physics->GetAxis ( );
}

/*
================
rvClientEntity::GetPhysics
================
*/
idPhysics* rvClientEntity::GetPhysics ( void ) const {
	return NULL;
}

/*
================
rvClientEntity::Collide
================
*/
bool rvClientEntity::Collide ( const trace_t &collision, const idVec3 &velocity ) {
	return false;
}

/*
===============================================================================

rvClientPhysics

===============================================================================
*/

CLASS_DECLARATION( idEntity, rvClientPhysics )
END_CLASS

/*
=====================
rvClientPhysics::Spawn
=====================
*/
void rvClientPhysics::Spawn( void ) {
}

/*
=====================
rvClientPhysics::Spawn
=====================
*/
bool rvClientPhysics::Collide( const trace_t &collision, const idVec3 &velocity ) {
	assert ( currentEntityNumber >= 0 && currentEntityNumber < MAX_CENTITIES );
	
	rvClientEntity* cent;
	cent = gameLocal.clientEntities [ currentEntityNumber ];
	if ( cent ) {
		return cent->Collide ( collision, velocity );
	}
	
	return false;
}
