

#ifndef	__AIMASSIST_H__
#define	__AIMASSIST_H__

/*
================================================================================================
Contains the AimAssist declaration.
================================================================================================
*/

class idEntity;
class idPlayer;

/*
================================================
idAimAssist modifies the angle of Weapon firing to help the Player 
hit a Target.
================================================
*/
class idAimAssist {
public:

				idAimAssist() : angleCorrection( ang_zero ), frictionScalar( 1.0f ), lastTargetPos( vec3_zero ), player( NULL ) {}

	void		Init( idPlayer * player );
	void		Update();
	void		GetAngleCorrection( idAngles & correction ) const { correction = angleCorrection; }
	float		GetFrictionScalar () const { return frictionScalar; }

	idEntity *	GetLastTarget() { return targetEntity; }
	idEntity *	FindAimAssistTarget( idVec3 & targetPos );

private:
	void		UpdateNewAimAssist();
	float		ComputeEntityAimAssistScore( const idVec3 & targetPos, const idVec3 & cameraPos, const idMat3 & cameraAxis );
	bool		ComputeTargetPos( idEntity * pTarget, idVec3 & primaryTargetPos, idVec3 & secondaryTargetPos );
	float		ComputeFrictionRadius( float distanceToTarget );
	void		UpdateAdhesion( idEntity * pTarget, const idVec3 & targetPos);
	void		UpdateFriction( idEntity * pTarget, const idVec3 & targetPos);

	idPlayer *				player;						// player associated with this object
	idAngles				angleCorrection;			// the angle delta to apply for aim assistance
	float					frictionScalar;				// friction scalar
	idEntityPtr<idEntity>	targetEntity;				// the last target we had (updated every frame)
	idVec3					lastTargetPos;				// the last target position ( updated every frame );
};

#endif // !__AIMASSIST_H__
