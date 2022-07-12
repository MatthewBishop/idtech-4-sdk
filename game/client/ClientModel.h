//----------------------------------------------------------------
// ClientModel.h
//
// A non-interactive client-side model
//
// Copyright 2002-2004 Raven Software
//----------------------------------------------------------------

#ifndef __GAME_CLIENT_MODEL_H__
#define __GAME_CLIENT_MODEL_H__

class rvClientModel : public rvClientEntity {
public:

	CLASS_PROTOTYPE( rvClientModel );

	rvClientModel ( void );
	virtual ~rvClientModel ( void );
	
	virtual void			Spawn			( const idDict* spawnArgs );
	virtual void			Think			( void );
	
	renderEntity_t*			GetRenderEntity	( void );
	const char*				GetClassname	( void ) const;

	bool					SetCustomShader	( const char* shaderName );

	void					Save			( idSaveGame *savefile ) const;
	void					Restore			( idRestoreGame *savefile );

	void					FreeEntityDef	( void );

protected:
	void					Present			( void );

	renderEntity_t			renderEntity;
	int						entityDefHandle;

	idStr					classname;
};

ID_INLINE renderEntity_t* rvClientModel::GetRenderEntity ( void ) {
	return &renderEntity;
}

ID_INLINE const char* rvClientModel::GetClassname ( void ) const {
	return classname.c_str();
}


#endif // __GAME_CLIENT_MODEL_H__
