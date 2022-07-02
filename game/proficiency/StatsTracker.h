// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_PROFICIENCY_STATSTRACKER_H__
#define __GAME_PROFICIENCY_STATSTRACKER_H__

#define DEBUG_STATS

#include "../../sdnet/SDNetStatsManager.h"

enum statsRequestState_t {
	SR_EMPTY,
	SR_REQUESTING,
	SR_COMPLETED,
	SR_FAILED,
};

typedef sdHandle< int, -1 > statHandle_t;

class sdPlayerStatEntry {
public:
	virtual						~sdPlayerStatEntry( void ) { ; }

	virtual void				IncreaseInteger( int playerIndex, int count ) { assert( false ); }
	virtual void				IncreaseFloat( int playerIndex, float count ) { assert( false ); }
	virtual void				Clear( int playerIndex ) = 0;
	virtual void				Display( const char* name ) const = 0;
	virtual void				Write( idFile* fp, int playerIndex, const char* name ) const = 0;
	virtual bool				Write( sdNetStatKeyValList& kvList, int playerIndex, const char* name, bool failOnBlank ) const = 0;
	virtual void				SetValue( int playerIndex, const sdNetStatKeyValue& value ) = 0;
	virtual float				GetValue( int playerIndex ) = 0;
};

typedef sdFactory< sdPlayerStatEntry > statFactory_t;

class sdPlayerStatEntry_Integer : public sdPlayerStatEntry {
public:
								sdPlayerStatEntry_Integer( void );

	virtual void				IncreaseInteger( int playerIndex, int count );
	virtual void				Clear( int playerIndex );
	virtual void				Display( const char* name ) const;
	virtual void				Write( idFile* fp, int playerIndex, const char* name ) const;
	virtual bool				Write( sdNetStatKeyValList& kvList, int playerIndex, const char* name, bool failOnBlank ) const;
	virtual void				SetValue( int playerIndex, const sdNetStatKeyValue& value );
	virtual float				GetValue( int playerIndex ) { return ( float )values[ playerIndex ]; }

private:
	int							values[ MAX_CLIENTS ];
};

class sdPlayerStatEntry_Float : public sdPlayerStatEntry {
public:
								sdPlayerStatEntry_Float( void );

	virtual void				IncreaseFloat( int playerIndex, float count );
	virtual void				Clear( int playerIndex );
	virtual void				Display( const char* name ) const;
	virtual void				Write( idFile* fp, int playerIndex, const char* name ) const;
	virtual bool				Write( sdNetStatKeyValList& kvList, int playerIndex, const char* name, bool failOnBlank ) const;
	virtual void				SetValue( int playerIndex, const sdNetStatKeyValue& value );
	virtual float				GetValue( int playerIndex ) { return values[ playerIndex ]; }

private:
	float						values[ MAX_CLIENTS ];
};

class sdStatsTracker;

class sdStatsCommand {
public:
	virtual						~sdStatsCommand( void ) { ; }
	virtual bool				Run( sdStatsTracker& tracker, const idCmdArgs& args ) = 0;
	virtual void				CommandCompletion( sdStatsTracker& tracker, const idCmdArgs& args, argCompletionCallback_t callback ) { ; }
};

class sdStatsCommand_Request : public sdStatsCommand {
public:
	virtual						~sdStatsCommand_Request( void ) { ; }
	virtual bool				Run( sdStatsTracker& tracker, const idCmdArgs& args );
};

class sdStatsCommand_Get : public sdStatsCommand {
public:
	virtual						~sdStatsCommand_Get( void ) { ; }
	virtual bool				Run( sdStatsTracker& tracker, const idCmdArgs& args );
};

class sdStatsCommand_Display : public sdStatsCommand {
public:
	virtual						~sdStatsCommand_Display( void ) { ; }
	virtual bool				Run( sdStatsTracker& tracker, const idCmdArgs& args );
	virtual void				CommandCompletion( sdStatsTracker& tracker, const idCmdArgs& args, argCompletionCallback_t callback );
};

class sdStatsTracker {
public:
								sdStatsTracker( void );
								~sdStatsTracker( void );
								
	statHandle_t				AllocStat( const char* name, const char* typeName );
	statHandle_t				GetStat( const char* name ) const;
	sdPlayerStatEntry*			GetStat( statHandle_t handle ) const;
	void						DisplayStat( statHandle_t handle ) const;
	void						Restore( int playerIndex );

	// Server
	void						CancelStatsRequest( int playerIndex );
	void						AddStatsRequest( int playerIndex );
	void						AcknowledgeStatsReponse( int playerIndex );

	// Client
	void						OnServerStatsRequestMessage( const idBitMsg& msg );
	void						OnServerStatsRequestCancelled( void );
	void						OnServerStatsRequestCompleted( void );

	void						Clear( void );
	void						Clear( int playerIndex );

	int							GetNumStats( void ) const { return stats.Num(); }
	// Gordon: This is only really used internally, so i'm leaving it unsafe, please guard this if you decide to use it externally
	const char*					GetStatName( statHandle_t handle ) const { return stats[ handle ]->GetName(); }

	void						Write( int playerIndex, const char* name );

	bool						StartStatsRequest( bool globalOnly = false );
	void						UpdateStatsRequests( void );

	statsRequestState_t			GetLocalStatsRequestState( void ) const { return requestState; }
	const sdNetStatKeyValList&	GetLocalStats( void ) const { return completeStats; }

	const idHashIndex&			GetLocalStatsHash( void ) const { return completeStatsHash; }
	
	const sdNetStatKeyValue*	GetLocalStat( const char* name ) const;

	static void					Init( void );

	static void					HandleCommand( const idCmdArgs& args );
	static void					CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback );

private:
	// Server
	void						ProcessLocalStats( int playerIndex );
	void						ProcessRemoteStats( int playerIndex );
	
	// Client
	void						UpdateStatsRequest( void );
	void						OnServerStatsRequestMessage( const sdNetStatKeyValList& list );
	void						OnStatsRequestFinished( void );

	sdNetStatKeyValue*			GetLocalStat( const char* name );


	class sdStatEntry {
	public:
								sdStatEntry( const char* _name
											, sdPlayerStatEntry* _entry
#ifdef DEBUG_STATS
											, const char* _typeName
#endif // DEBUG_STATS
											) : name( _name )
												, entry( _entry )
#ifdef DEBUG_STATS
												, typeName( _typeName )
#endif // DEBUG_STATS
											{ ; }

								~sdStatEntry( void ) { delete entry; }

		void					Clear( int playerIndex ) { entry->Clear( playerIndex ); }
		void					Display( void ) const { entry->Display( name ); }
		void					Write( idFile* fp, int playerIndex ) const { entry->Write( fp, playerIndex, name ); }
		bool					Write( sdNetStatKeyValList& kvList, int playerIndex, bool failOnBlank = false ) const { return entry->Write( kvList, playerIndex, name, failOnBlank ); }

		sdPlayerStatEntry*		GetEntry( void ) const { return entry; }

		const char*				GetName( void ) const { return name; }

#ifdef DEBUG_STATS
		const char*				GetTypeName( void ) const { return typeName; }
#endif // DEBUG_STATS
		


	private:
		sdPlayerStatEntry*		entry;
		idStr					name;
#ifdef DEBUG_STATS
		idStr					typeName;
#endif // DEBUG_STATS
	};

	sdNetTask*					requestTask;

	bool						requestedStatsValid;
	sdNetStatKeyValList			requestedStats;

	bool						serverStatsValid;
	sdNetStatKeyValList			serverStats;

	sdNetStatKeyValList			completeStats;
	idHashIndex					completeStatsHash;

	int							playerRequestState[ MAX_CLIENTS ];
	bool						playerRequestWaiting[ MAX_CLIENTS ];

	statsRequestState_t			requestState;

	idList< sdStatEntry* >		stats;
	idHashIndex					statHash;

	static statFactory_t				s_statFactory;
	static idHashMap< sdStatsCommand* >	s_commands;
};

typedef sdSingleton< sdStatsTracker > sdGlobalStatsTracker;

#endif // __GAME_PROFICIENCY_STATSTRACKER_H__
