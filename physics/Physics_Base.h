

#ifndef __PHYSICS_BASE_H__
#define __PHYSICS_BASE_H__

/*
===============================================================================

	Physics base for a moving object using one or more collision models.

===============================================================================
*/

#define contactEntity_t		idEntityPtr<idEntity>

class idPhysics_Base : public idPhysics {

public:
	CLASS_PROTOTYPE( idPhysics_Base );

							idPhysics_Base();
							~idPhysics_Base();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

public:	// common physics interface

	void					SetSelf( idEntity *e );

	void					SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true );
	idClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels() const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	void					SetClipMask( int mask, int id = -1 );
	int						GetClipMask( int id = -1 ) const;

	const idBounds &		GetBounds( int id = -1 ) const;
	const idBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime() const;

	void					GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse );
	void					AddForce( const int id, const idVec3 &point, const idVec3 &force );
	void					Activate();
	void					PutToRest();
	bool					IsAtRest() const;
	int						GetRestStartTime() const;
	bool					IsPushable() const;

	void					SaveState();
	void					RestoreState();

	void					SetOrigin( const idVec3 &newOrigin, int id = -1 );
	void					SetAxis( const idMat3 &newAxis, int id = -1 );

	void					Translate( const idVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	const idVec3 &			GetOrigin( int id = 0 ) const;
	const idMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 );
	void					SetAngularVelocity( const idVec3 &newAngularVelocity, int id = 0 );

	const idVec3 &			GetLinearVelocity( int id = 0 ) const;
	const idVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					SetGravity( const idVec3 &newGravity );
	const idVec3 &			GetGravity() const;
	const idVec3 &			GetGravityNormal() const;

	void					ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const;
	void					ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const;
	int						ClipContents( const idClipModel *model ) const;

	void					DisableClip();
	void					EnableClip();

	void					UnlinkClip();
	void					LinkClip();

	bool					EvaluateContacts();
	int						GetNumContacts() const;
	const contactInfo_t &	GetContact( int num ) const;
	void					ClearContacts();
	void					AddContactEntity( idEntity *e );
	void					RemoveContactEntity( idEntity *e );

	bool					HasGroundContacts() const;
	bool					IsGroundEntity( int entityNum ) const;
	bool					IsGroundClipModel( int entityNum, int id ) const;

	void					SetPushed( int deltaTime );
	const idVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	const idVec3 &			GetPushedAngularVelocity( const int id = 0 ) const;

	void					SetMaster( idEntity *master, const bool orientated = true );

	const trace_t *			GetBlockingInfo() const;
	idEntity *				GetBlockingEntity() const;

	int						GetLinearEndTime() const;
	int						GetAngularEndTime() const;

	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );

protected:
	idEntity *				self;					// entity using this physics object
	int						clipMask;				// contents the physics object collides with
	idVec3					gravityVector;			// direction and magnitude of gravity
	idVec3					gravityNormal;			// normalized direction of gravity
	idList<contactInfo_t>	contacts;				// contacts with other physics objects
	idList<contactEntity_t>	contactEntities;		// entities touching this physics object

protected:
							// add ground contacts for the clip model
	void					AddGroundContacts( const idClipModel *clipModel );
							// add contact entity links to contact entities
	void					AddContactEntitiesForContacts();
							// active all contact entities
	void					ActivateContactEntities();
							// returns true if the whole physics object is outside the world bounds
	bool					IsOutsideWorld() const;
							// draw linear and angular velocity
	void					DrawVelocity( int id, float linearScale, float angularScale ) const;
};

#endif /* !__PHYSICS_BASE_H__ */
