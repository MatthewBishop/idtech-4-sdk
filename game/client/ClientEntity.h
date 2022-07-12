//----------------------------------------------------------------
// ClientEntity.h
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef __GAME_CLIENT_ENTITY_H__
#define __GAME_CLIENT_ENTITY_H__

class rvClientEntity : public idClass {
public:

	ABSTRACT_PROTOTYPE( rvClientEntity );

	rvClientEntity ( void );
	virtual ~rvClientEntity ( void );

	virtual void		Present				( void );
	virtual void		Think				( void );
	virtual idPhysics*	GetPhysics			( void ) const;
	virtual bool		Collide				( const trace_t &collision, const idVec3 &velocity );

	void				SetOrigin			( const idVec3& origin );
	void				SetAxis				( const idMat3& axis );
	const idVec3&		GetOrigin			( void );
	const idMat3&		GetAxis				( void );
	
	void				Bind				( idEntity* master, jointHandle_t joint = INVALID_JOINT );
	void				Unbind				( void );

	virtual bool		IsClient			( void ) const;
	virtual void		DrawDebugInfo		( void ) const;

	int					StartSoundShader	( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags );

	// I'm guessing someone may decide to derive from rvClientEntity, so this virtual.
	virtual	size_t		Size				( void ) const;

	virtual void		FreeEntityDef		( void );

	const idEntityPtr<idEntity>&			GetBindMaster( void ) const;

	void				Save				( idSaveGame *savefile ) const;
	void				Restore				( idRestoreGame *savefile );

protected:

	void				RunPhysics			( void );

	virtual void		UpdateBind			( void );
	void				UpdateSound			( void );

public:
	
	int							entityNumber;

	idLinkList<rvClientEntity>	spawnNode;
	idLinkList<rvClientEntity>	bindNode;

protected:

	idVec3						worldOrigin;
	idMat3						worldAxis;

	idEntityPtr<idEntity>		bindMaster;
	idVec3						bindOrigin;
	idMat3						bindAxis;
	jointHandle_t				bindJoint;
	
	refSound_t					refSound;
};

ID_INLINE const idVec3& rvClientEntity::GetOrigin ( void ) {
	return worldOrigin;
}

ID_INLINE const idMat3& rvClientEntity::GetAxis ( void ) {
	return worldAxis;
}

ID_INLINE const idEntityPtr<idEntity>& rvClientEntity::GetBindMaster( void ) const {
	return bindMaster;
}

//============================================================================

template< class type >
class rvClientEntityPtr {
public:
							rvClientEntityPtr();

	rvClientEntityPtr<type> &	operator=( type* cent );

	int						GetSpawnId		( void ) const { return spawnId; }
	bool					SetSpawnId		( int id );
	bool					UpdateSpawnId	( void );

	bool					IsValid			( void ) const;
	type *					GetEntity		( void ) const;
	int						GetEntityNum	( void ) const;

	type *					operator->		( void ) const { return GetEntity ( ); }			
	operator				type*			( void ) const { return GetEntity ( ); }

	void					Save			( idSaveGame *savefile ) const;
	void					Restore			( idRestoreGame *savefile );

private:
	int						spawnId;
};

template< class type >
ID_INLINE rvClientEntityPtr<type>::rvClientEntityPtr() {
	spawnId = 0;
}

template< class type >
ID_INLINE rvClientEntityPtr<type> &rvClientEntityPtr<type>::operator=( type *cent ) {
	if ( cent == NULL ) {
		spawnId = 0;
	} else {
		spawnId = ( gameLocal.clientSpawnIds[cent->entityNumber] << CENTITYNUM_BITS ) | cent->entityNumber;
	}
	return *this;
}

template< class type >
ID_INLINE bool rvClientEntityPtr<type>::SetSpawnId( int id ) {
	if ( id == spawnId ) {
		return false;
	}
	if ( ( id >> CENTITYNUM_BITS ) == gameLocal.clientSpawnIds[ id & ( ( 1 << CENTITYNUM_BITS ) - 1 ) ] ) {
		spawnId = id;
		return true;
	}
	return false;
}

template< class type >
ID_INLINE bool rvClientEntityPtr<type>::IsValid( void ) const {
	return ( gameLocal.clientSpawnIds[ spawnId & ( ( 1 << CENTITYNUM_BITS ) - 1 ) ] == ( spawnId >> CENTITYNUM_BITS ) );
}

template< class type >
ID_INLINE type *rvClientEntityPtr<type>::GetEntity( void ) const {
	int entityNum = spawnId & ( ( 1 << CENTITYNUM_BITS ) - 1 );
	if ( ( gameLocal.clientSpawnIds[ entityNum ] == ( spawnId >> CENTITYNUM_BITS ) ) ) {
		return static_cast<type *>( gameLocal.clientEntities[ entityNum ] );
	}
	return NULL;
}

template< class type >
ID_INLINE int rvClientEntityPtr<type>::GetEntityNum( void ) const {
	return ( spawnId & ( ( 1 << CENTITYNUM_BITS ) - 1 ) );
}

template< class type >
ID_INLINE void rvClientEntityPtr<type>::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( spawnId );
}

template< class type >
ID_INLINE void rvClientEntityPtr<type>::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( spawnId );
}

/*
===============================================================================

rvClientPhysics

===============================================================================
*/

class rvClientPhysics : public idEntity {
public:

	CLASS_PROTOTYPE( rvClientPhysics );

						rvClientPhysics( void ) { currentEntityNumber = -1; }

	void				Spawn( void );

	virtual bool		Collide( const trace_t &collision, const idVec3 &velocity );
	
	int		currentEntityNumber;
};


#endif // __GAME_CLIENT_ENTITY_H__
