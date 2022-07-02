// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "StatsTracker.h"

#include "../rules/GameRules.h"
#include "../Player.h"

#include "../../sdnet/SDNet.h"
#include "../../sdnet/SDNetStatsManager.h"
#include "../../sdnet/SDNetAccount.h"
#include "../../sdnet/SDNetUser.h"

/*
===============================================================================

	sdPlayerStatEntry_Float

===============================================================================
*/

/*
================
sdPlayerStatEntry_Float::sdPlayerStatEntry_Float
================
*/
sdPlayerStatEntry_Float::sdPlayerStatEntry_Float( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		Clear( i );
	}
}

/*
================
sdPlayerStatEntry_Float::Clear
================
*/
void sdPlayerStatEntry_Float::Clear( int playerIndex ) {
	values[ playerIndex ] = 0;
}

/*
================
sdPlayerStatEntry_Float::IncreaseFloat
================
*/
void sdPlayerStatEntry_Float::IncreaseFloat( int playerIndex, float count ) {
	if ( gameLocal.rules->IsGameOn() ) {
		values[ playerIndex ] += count;
	}
}

/*
================
sdPlayerStatEntry_Float::Display
================
*/
void sdPlayerStatEntry_Float::Display( const char* name ) const {
	gameLocal.Printf( "%s\n", name );
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		gameLocal.Printf( "%f ", values[ i ] );
	}
	gameLocal.Printf( "\n" );
}

/*
================
sdPlayerStatEntry_Float::Write
================
*/
void sdPlayerStatEntry_Float::Write( idFile* fp, int playerIndex, const char* name ) const {
	fp->WriteFloatString( "%s: %f\n", name, values[ playerIndex ] );
}

/*
================
sdPlayerStatEntry_Float::Write
================
*/
bool sdPlayerStatEntry_Float::Write( sdNetStatKeyValList& kvList, int playerIndex, const char* name, bool failOnBlank ) const {
	if ( failOnBlank && values[ playerIndex ] == 0.f ) {
		return false;
	}

	idStrPool* globalKeys;
	idStrPool* globalValues;

	idDict::GetGlobalPools( globalKeys, globalValues );

	sdNetStatKeyValue kv;

	kv.key = globalKeys->AllocString( name );
	kv.type = sdNetStatKeyValue::SVT_FLOAT;
	kv.val.f = values[ playerIndex ];

	kvList.Append( kv );

	return true;
}

/*
================
sdPlayerStatEntry_Float::SetValue
================
*/
void sdPlayerStatEntry_Float::SetValue( int playerIndex, const sdNetStatKeyValue& value ) {
	if ( value.type == sdNetStatKeyValue::SVT_FLOAT ) {
		values[ playerIndex ] = value.val.f;
	} else {
		assert( false );
	}
}

/*
===============================================================================

	sdPlayerStatEntry_Integer

===============================================================================
*/

/*
================
sdPlayerStatEntry_Integer::sdPlayerStatEntry_Integer
================
*/
sdPlayerStatEntry_Integer::sdPlayerStatEntry_Integer( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		Clear( i );
	}
}

/*
================
sdPlayerStatEntry_Integer::Clear
================
*/
void sdPlayerStatEntry_Integer::Clear( int playerIndex ) {
	values[ playerIndex ] = 0;
}

/*
================
sdPlayerStatEntry_Integer::IncreaseInteger
================
*/
void sdPlayerStatEntry_Integer::IncreaseInteger( int playerIndex, int count ) {
	if ( gameLocal.rules->IsGameOn() ) {
		values[ playerIndex ] += count;
	}
}

/*
================
sdPlayerStatEntry_Integer::Display
================
*/
void sdPlayerStatEntry_Integer::Display( const char* name ) const {
	gameLocal.Printf( "%s\n", name );
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		gameLocal.Printf( "%i ", values[ i ] );
	}
	gameLocal.Printf( "\n" );
}

/*
================
sdPlayerStatEntry_Integer::Write
================
*/
void sdPlayerStatEntry_Integer::Write( idFile* fp, int playerIndex, const char* name ) const {
	fp->WriteFloatString( "%s: %i\n", name, values[ playerIndex ] );
}

/*
================
sdPlayerStatEntry_Integer::Write
================
*/
bool sdPlayerStatEntry_Integer::Write( sdNetStatKeyValList& kvList, int playerIndex, const char* name, bool failOnBlank ) const {
	if ( failOnBlank && values[ playerIndex ] == 0 ) {
		return false;
	}

	idStrPool* globalKeys;
	idStrPool* globalValues;

	idDict::GetGlobalPools( globalKeys, globalValues );

	sdNetStatKeyValue kv;

	kv.key = globalKeys->AllocString( name );
	kv.type = sdNetStatKeyValue::SVT_INT;
	kv.val.i = values[ playerIndex ];

	kvList.Append( kv );

	return true;
}

/*
================
sdPlayerStatEntry_Integer::SetValue
================
*/
void sdPlayerStatEntry_Integer::SetValue( int playerIndex, const sdNetStatKeyValue& value ) {
	if ( value.type == sdNetStatKeyValue::SVT_INT ) {
		values[ playerIndex ] = value.val.i;
	} else {
		assert( false );
	}
}

/*
===============================================================================

	sdStatsCommand_Request

===============================================================================
*/

/*
================
sdStatsCommand_Request::Run
================
*/
bool sdStatsCommand_Request::Run( sdStatsTracker& tracker, const idCmdArgs& args ) {
	return sdGlobalStatsTracker::GetInstance().StartStatsRequest();
}

/*
===============================================================================

	sdStatsCommand_Get

===============================================================================
*/

/*
================
sdStatsCommand_Get::Run
================
*/
bool sdStatsCommand_Get::Run( sdStatsTracker& tracker, const idCmdArgs& args ) {
	if ( args.Argc() < 3 ) {
		gameLocal.Printf( "Insufficient Arguments\n" );
		return false;
	}

	const char* name = args.Argv( 2 );

	statHandle_t handle = tracker.GetStat( name );
	if ( !handle.IsValid() ) {
		gameLocal.Printf( "Failed To Find Stat '%s'\n", name );
		return false;
	}

	gameLocal.Printf( "Found Stat Handle %i\n", handle );

	return true;
}

/*
===============================================================================

	sdStatsCommand_Display

===============================================================================
*/

/*
================
sdStatsCommand_Display::Run
================
*/
bool sdStatsCommand_Display::Run( sdStatsTracker& tracker, const idCmdArgs& args ) {
	if ( args.Argc() < 3 ) {
		gameLocal.Printf( "Insufficient Arguments\n" );
		return false;
	}

	const char* name = args.Argv( 2 );

	statHandle_t handle = tracker.GetStat( name );
	if ( !handle.IsValid() ) {
		gameLocal.Printf( "Failed To Find Stat '%s'\n", name );
		return false;
	}

	tracker.DisplayStat( handle );

	return true;
}

/*
================
sdStatsCommand_Display::CommandCompletion
================
*/
void sdStatsCommand_Display::CommandCompletion( sdStatsTracker& tracker, const idCmdArgs& args, argCompletionCallback_t callback ) {
	for ( int i = 0; i < tracker.GetNumStats(); i++ ) {		
		callback( va( "%s %s %s", args.Argv( 0 ), args.Argv( 1 ), tracker.GetStatName( i ) ) );
	}
}

/*
===============================================================================

	sdStatsTracker

===============================================================================
*/

statFactory_t					sdStatsTracker::s_statFactory;
idHashMap< sdStatsCommand* >	sdStatsTracker::s_commands;

/*
================
sdStatsTracker::sdStatsTracker
================
*/
sdStatsTracker::sdStatsTracker( void ) {
	requestState = SR_EMPTY;
	requestTask = NULL;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		playerRequestState[ i ] = -1;
	}
}

/*
================
sdStatsTracker::sdStatsTracker
================
*/
sdStatsTracker::~sdStatsTracker( void ) {
	stats.DeleteContents( true );
	statHash.Clear();
}

/*
================
sdStatsTracker::Init
================
*/
void sdStatsTracker::Init( void ) {
	s_statFactory.RegisterType( "int",		statFactory_t::Allocator< sdPlayerStatEntry_Integer > );
	s_statFactory.RegisterType( "float",	statFactory_t::Allocator< sdPlayerStatEntry_Float > );

	s_commands.Set( "request",	new sdStatsCommand_Request() );
	s_commands.Set( "get",		new sdStatsCommand_Get() );
	s_commands.Set( "display",	new sdStatsCommand_Display() );
}

/*
================
sdStatsTracker::GetStat
================
*/
statHandle_t sdStatsTracker::GetStat( const char* name ) const {
	int hashKey = idStr::Hash( name );
	for ( int index = statHash.GetFirst( hashKey ); index != idHashIndex::NULL_INDEX; index = statHash.GetNext( index ) ) {
		if ( idStr::Icmp( stats[ index ]->GetName(), name ) != 0 ) {
			continue;
		}

		return index;
	}

	return statHandle_t();
}

/*
================
sdStatsTracker::AllocStat
================
*/
statHandle_t sdStatsTracker::AllocStat( const char* name, const char* typeName ) {
	int hashKey = idStr::Hash( name );
	for ( int index = statHash.GetFirst( hashKey ); index != idHashIndex::NULL_INDEX; index = statHash.GetNext( index ) ) {
		if ( idStr::Icmp( stats[ index ]->GetName(), name ) != 0 ) {
			continue;
		}

#ifdef DEBUG_STATS
		if ( idStr::Icmp( stats[ index ]->GetTypeName(), typeName ) != 0 ) {
			gameLocal.Error( "sdStatsTracker::AllocStat Type Mismatch ( '%s' v. '%s' ) On Re-Allocation of '%s'", stats[ index ]->GetTypeName(), typeName, name );
		}
#endif // DEBUG_STATS

		return index;
	}

	statHandle_t handle = stats.Num();

	sdPlayerStatEntry* stat = s_statFactory.CreateType( typeName );
	if ( !stat ) {
		gameLocal.Error( "sdStatsTracker::AllocStat Failed to Alloc Stat '%s' of type '%s'", name, typeName );
		return statHandle_t();
	}

	sdStatEntry* entry = new sdStatEntry( name
										, stat
#ifdef DEBUG_STATS
										, typeName
#endif // DEBUG_STATS
										);
	stats.Alloc() = entry;
	statHash.Add( hashKey, handle );

	return handle;
}

/*
================
sdStatsTracker::Clear
================
*/
void sdStatsTracker::Clear( void ) {
	gameLocal.Printf( "All Stats Cleared\n" );

	for ( int i = 0; i < stats.Num(); i++ ) {
		for ( int p = 0; p < MAX_CLIENTS; p++ ) {
			stats[ i ]->Clear( p );
		}
	}
}

/*
================
sdStatsTracker::Clear
================
*/
void sdStatsTracker::Clear( int playerIndex ) {
	assert( playerIndex >= 0 && playerIndex < MAX_CLIENTS );

	for ( int i = 0; i < stats.Num(); i++ ) {
		stats[ i ]->Clear( playerIndex );
	}
}

/*
================
sdStatsTracker::DisplayStat
================
*/
void sdStatsTracker::DisplayStat( statHandle_t handle ) const {
	assert( handle.IsValid() );

	stats[ handle ]->Display();
}

/*
================
sdStatsTracker::GetStat
================
*/
sdPlayerStatEntry* sdStatsTracker::GetStat( statHandle_t handle ) const {
	if ( !handle.IsValid() ) {
		return NULL;
	}

	return stats[ handle ]->GetEntry();
}

/*
================
sdStatsTracker::HandleCommand
================
*/
void sdStatsTracker::HandleCommand( const idCmdArgs& args ) {
	if ( args.Argc() < 2 ) {
		gameLocal.Printf( "Insufficient Arguments\n" );
		return;
	}

	sdStatsCommand** command = NULL;
	if ( !s_commands.Get( args.Argv( 1 ), &command ) ) {
		gameLocal.Printf( "Unknown Command '%s'\n", args.Argv( 1 ) );
		return;
	}	

	if ( !( *command )->Run( sdGlobalStatsTracker::GetInstance(), args ) ) {
		gameLocal.Printf( "Failed to Complete Command Properly\n" );
		return;
	}
}

/*
================
sdStatsTracker::CommandCompletion
================
*/
void sdStatsTracker::CommandCompletion( const idCmdArgs& args, argCompletionCallback_t callback ) {
	const char* cmd = args.Argv( 1 );

	for ( int i = 0; i < s_commands.Num(); i++ ) {
		const char* name = s_commands.GetKeyList()[ i ].c_str();

		if ( idStr::Icmp( cmd, name ) != 0 ) {
			continue;
		}

		s_commands.GetValList()[ i ]->CommandCompletion( sdGlobalStatsTracker::GetInstance(), args, callback );
	}

	int cmdLength = idStr::Length( cmd );
	for ( int i = 0; i < s_commands.Num(); i++ ) {
		const char* name = s_commands.GetKeyList()[ i ].c_str();

		if ( idStr::Icmpn( cmd, name, cmdLength ) != 0 ) {
			continue;
		}

		callback( va( "%s %s", args.Argv( 0 ), name ) );
	}
}

idCVar g_writeStats( "g_writeStats", "0", CVAR_BOOL | CVAR_GAME | CVAR_NOCHEAT, "write stats txt files at the end of maps" );

/*
================
sdStatsTracker::Write
================
*/
void sdStatsTracker::Write( int playerIndex, const char* name ) {
	if ( g_writeStats.GetBool() ) {
		idStr temp = name;
		temp.CleanFilename();

		idFile* fp = NULL;

		for ( int i = 1; i < 9999; i++ ) {
			const char* fileName = va( "/stats/%s_%i.txt", temp.c_str(), i );
			fp = fileSystem->OpenFileRead( fileName, "fs_userpath" );
			if ( fp != NULL ) {
				fileSystem->CloseFile( fp );
				continue;
			}

			fp = fileSystem->OpenFileWrite( fileName );
			break;
		}

		if ( fp != NULL ) {
			for ( int i = 0; i < stats.Num(); i++ ) {
				stats[ i ]->Write( fp, playerIndex );
			}

			fileSystem->CloseFile( fp );
		}
	}

#if !defined( SD_DEMO_BUILD )
	if ( networkService->GetDedicatedServerState() == sdNetService::DS_ONLINE ) {
		sdNetClientId netClientId;
		networkSystem->ServerGetClientNetId( playerIndex, netClientId );

		if ( netClientId.IsValid() ) {
			sdNetStatKeyValList kvList;
			for ( int i = 0; i < stats.Num(); i++ ) {
				stats[ i ]->Write( kvList, playerIndex );
			}

			networkService->GetStatsManager().WriteDictionary( netClientId, kvList );
		}
	}
#endif /* !SD_DEMO_BUILD */
}

/*
================
sdStatsTracker::Restore
================
*/
void sdStatsTracker::Restore( int playerIndex ) {
#if !defined( SD_DEMO_BUILD )
	if ( networkService->GetDedicatedServerState() == sdNetService::DS_ONLINE ) {
		sdNetClientId netClientId;
		networkSystem->ServerGetClientNetId( playerIndex, netClientId );

		if ( netClientId.IsValid() ) {
			sdNetStatKeyValList kvList;
			networkService->GetStatsManager().ReadCachedDictionary( netClientId, kvList );

			for ( int i = 0; i < kvList.Num(); i++ ) {
				if ( kvList[ i ].key == NULL ) {
					continue;
				}

				const char* statName = kvList[ i ].key->c_str();

				statHandle_t handle = GetStat( statName );
				if ( !handle.IsValid() ) {
					continue;
				}

				sdPlayerStatEntry* entry = GetStat( handle );
				entry->SetValue( playerIndex, kvList[ i ] );
			}
		}
	}
#endif /* !SD_DEMO_BUILD */
}

/*
================
sdStatsTracker::ProcessLocalStats
================
*/
void sdStatsTracker::ProcessLocalStats( int playerIndex ) {
	sdNetStatKeyValList list;
	for ( int i = 0; i < stats.Num(); i++ ) {
		stats[ i ]->Write( list, playerIndex );
	}
	OnServerStatsRequestMessage( list );
	playerRequestState[ playerIndex ] = -1;

	OnServerStatsRequestCompleted();
}

/*
================
sdStatsTracker::ProcessRemoteStats
================
*/
void sdStatsTracker::ProcessRemoteStats( int playerIndex ) {
	const int MAX_SINGLE_SEND_COUNT = 30;

	int numLeft = stats.Num() - playerRequestState[ playerIndex ];
	int numSend = Min( numLeft, MAX_SINGLE_SEND_COUNT );

	sdNetStatKeyValList list;
	int start = playerRequestState[ playerIndex ];
	int nextStatToWrite = start;

	for ( int i = 0; start + i < stats.Num() && list.Num() < numSend; i++ ) {
		int index = start + i;
		stats[ index ]->Write( list, playerIndex, true );
		nextStatToWrite = index + 1;
	}

	sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_STATSMESSAGE );
	msg.WriteLong( list.Num() );
	for ( int i = 0; i < list.Num(); i++ ) {
		msg.WriteString( list[ i ].key->c_str() );
		msg.WriteByte( list[ i ].type );
		switch ( list[ i ].type ) {
			case sdNetStatKeyValue::SVT_INT:
				msg.WriteLong( list[ i ].val.i );
				break;
			case sdNetStatKeyValue::SVT_FLOAT:
				msg.WriteFloat( list[ i ].val.f );
				break;
		}
	}

	msg.Send( playerIndex );

	playerRequestState[ playerIndex ] = nextStatToWrite;
	if ( playerRequestState[ playerIndex ] == stats.Num() ) {
		sdReliableServerMessage finishMsg( GAME_RELIABLE_SMESSAGE_STATSFINIHED );
		finishMsg.WriteBool( true );
		finishMsg.Send( playerIndex );

		playerRequestState[ playerIndex ] = -1;
	}
}

/*
================
sdStatsTracker::UpdateStatsRequests
================
*/
void sdStatsTracker::UpdateStatsRequests( void ) {
	UpdateStatsRequest();

	if ( !gameLocal.isClient ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			if ( playerRequestState[ i ] == -1 || playerRequestWaiting[ i ] ) {
				continue;
			}

			idPlayer* player = gameLocal.GetClient( i );
			if ( player == NULL ) {
				playerRequestState[ i ] = -1;
				continue;
			}

			if ( gameLocal.IsLocalPlayer( player ) ) {
				ProcessLocalStats( i );
			} else {
				ProcessRemoteStats( i );
			}
		}
	}
}

/*
================
sdStatsTracker::AcknowledgeStatsReponse
================
*/
void sdStatsTracker::AcknowledgeStatsReponse( int playerIndex ) {
	playerRequestWaiting[ playerIndex ] = false;
}

/*
================
sdStatsTracker::AddStatsRequest
================
*/
void sdStatsTracker::AddStatsRequest( int playerIndex ) {
	if ( playerRequestState[ playerIndex ] != -1 ) { // already doing stuff
		return;
	}
	playerRequestState[ playerIndex ] = 0;
	playerRequestWaiting[ playerIndex ] = false;
}

/*
================
sdStatsTracker::CancelStatsRequest
================
*/
void sdStatsTracker::CancelStatsRequest( int playerIndex ) {
	if ( playerRequestState[ playerIndex ] == -1 ) { // not doing anything anyway
		return;
	}

	playerRequestState[ playerIndex ] = -1;

	idPlayer* player = gameLocal.GetClient( playerIndex );
	if ( player != NULL ) {
		if ( gameLocal.IsLocalPlayer( player ) ) {
			OnServerStatsRequestCancelled();
		} else {
			sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_STATSFINIHED );
			msg.WriteBool( false );
			msg.Send( playerIndex );
		}
	}
}


/*
================
sdStatsTracker::OnServerStatsRequestCancelled
================
*/
void sdStatsTracker::OnServerStatsRequestCancelled( void ) {
	if ( requestState != SR_REQUESTING ) {
		return;
	}

	if ( requestTask != NULL ) {
		networkService->FreeTask( requestTask );
		requestTask = NULL;
	}

	requestedStatsValid = false;
	serverStatsValid = false;
	requestState = SR_FAILED;
}

/*
================
sdStatsTracker::OnServerStatsRequestCompleted
================
*/
void sdStatsTracker::OnServerStatsRequestCompleted( void ) {
	if ( requestState != SR_REQUESTING ) {
		return;
	}

	serverStatsValid = true;
	if ( requestedStatsValid ) {
		OnStatsRequestFinished();
	}
}

/*
================
sdStatsTracker::OnServerStatsRequestMessage
================
*/
void sdStatsTracker::OnServerStatsRequestMessage( const idBitMsg& msg ) {
	idStrPool* globalKeys;
	idStrPool* globalValues;
	idDict::GetGlobalPools( globalKeys, globalValues );

	char buffer[ 2048 ];

	sdNetStatKeyValList list;
	int numEntries = msg.ReadLong();
	for ( int i = 0; i < numEntries; i++ ) {
		msg.ReadString( buffer, sizeof( buffer ) );		

		sdNetStatKeyValue kv;
		kv.type = ( sdNetStatKeyValue::statValueType )msg.ReadByte();
		kv.key = globalKeys->AllocString( buffer );
		switch ( kv.type ) {
			case sdNetStatKeyValue::SVT_FLOAT:
				kv.val.f = msg.ReadFloat();
				break;
			case sdNetStatKeyValue::SVT_INT:
				kv.val.i = msg.ReadLong();
				break;
		}

		list.Append( kv );
	}

	OnServerStatsRequestMessage( list );

	sdReliableClientMessage response( GAME_RELIABLE_CMESSAGE_ACKNOWLEDGESTATS );
	response.Send();
}

/*
================
sdStatsTracker::OnServerStatsRequestMessage
================
*/
void sdStatsTracker::OnServerStatsRequestMessage( const sdNetStatKeyValList& list ) {
	serverStats.Append( list );
}

idCVar g_useSimpleStats( "g_useSimpleStats", "0", CVAR_BOOL | CVAR_GAME, "only look up local server stats" );

/*
================
sdStatsTracker::StartStatsRequest
================
*/
bool sdStatsTracker::StartStatsRequest( bool globalOnly ) {
	if ( requestState == SR_REQUESTING ) {
		return false;
	}
	requestState = SR_FAILED;

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL && !globalOnly ) {
		return false;
	}

	assert( requestTask == NULL );

	serverStatsValid = globalOnly ? true : false;
	serverStats.SetNum( 0, false );

	requestedStatsValid = false;
	requestedStats.SetNum( 0, false );

	if ( g_useSimpleStats.GetBool() && !globalOnly ) {
		requestedStatsValid = true;
	} else {
#if !defined( SD_DEMO_BUILD )
		sdNetUser* activeUser = networkService->GetActiveUser();
		if ( activeUser != NULL ) {
			sdNetClientId clientId;
			activeUser->GetAccount().GetNetClientId( clientId );
			if ( clientId.IsValid() ) {
				requestTask = networkService->GetStatsManager().ReadDictionary( clientId, requestedStats );
			}
		}
#endif /* !SD_DEMO_BUILD */

		if ( requestTask == NULL ) {
			if ( !globalOnly ) {
				requestedStatsValid = true; // Gordon: just default to getting "server" stats if this stuff fails
			} else {
				return false;
			}
		}
	}

	requestState = SR_REQUESTING;

	if ( !globalOnly ) {
		if ( gameLocal.isClient ) {
			sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_REQUESTSTATS );
			msg.Send();
		} else {
			AddStatsRequest( localPlayer->entityNumber );
		}
	}

	return true;
}

/*
================
sdStatsTracker::UpdateStatsRequest
================
*/
void sdStatsTracker::UpdateStatsRequest( void ) {
	if ( requestState != SR_REQUESTING ) {
		return;
	}

	if ( requestTask != NULL ) {
		sdNetTask::taskStatus_e taskStatus = requestTask->GetState();
		if ( taskStatus == sdNetTask::TS_DONE ) {
			sdNetErrorCode_e errorCode = requestTask->GetErrorCode();
			
			if ( errorCode == SDNET_NO_ERROR ) {
				requestedStatsValid = true;
				if ( serverStatsValid ) {
					OnStatsRequestFinished();
				}
			} else {
				requestState = SR_FAILED;
			}

			networkService->FreeTask( requestTask );
			requestTask = NULL;
		}
	}
}

/*
================
sdStatsTracker::OnStatsRequestFinished
================
*/
void sdStatsTracker::OnStatsRequestFinished( void ) {
	requestState = SR_COMPLETED;

	completeStats.Clear();
	completeStatsHash.Clear();

	completeStats.SetNum( requestedStats.Num() );
	for( int i = 0; i < requestedStats.Num(); i++ ) {
		completeStats[ i ] = requestedStats[ i ];
		int hashKey = completeStatsHash.GenerateKey( completeStats[ i ].key->c_str(), false );
		completeStatsHash.Add( hashKey, i );
	}	

	for ( int i = 0; i < serverStats.Num(); i++ ) {
		sdNetStatKeyValue* kv = GetLocalStat( serverStats[ i ].key->c_str() );
		if( kv != NULL ) {
			switch( kv->type ) {
				case sdNetStatKeyValue::SVT_FLOAT:
					kv->val.f += serverStats[ i ].val.f;
					break;
				case sdNetStatKeyValue::SVT_INT:
					kv->val.i += serverStats[ i ].val.i;
					break;
			}
		} else {
			int hashKey = idStr::Hash( serverStats[ i ].key->c_str() );
			completeStatsHash.Add( hashKey, completeStats.Append( serverStats[ i ] ) );
		}	
	}	
}

/*
============
sdStatsTracker::GetLocalStat
============
*/
const sdNetStatKeyValue* sdStatsTracker::GetLocalStat( const char* name ) const {
	int hashKey = idStr::Hash( name );
	for ( int index = completeStatsHash.GetFirst( hashKey ); index != idHashIndex::NULL_INDEX; index = completeStatsHash.GetNext( index ) ) {
		if ( idStr::Icmp( completeStats[ index ].key->c_str(), name ) != 0 ) {
			continue;
		}

		return &completeStats[ index ];
	}

	return NULL;
}

/*
============
sdStatsTracker::GetLocalStat
============
*/
sdNetStatKeyValue* sdStatsTracker::GetLocalStat( const char* name ) {
	int hashKey = idStr::Hash( name );
	for ( int index = completeStatsHash.GetFirst( hashKey ); index != idHashIndex::NULL_INDEX; index = completeStatsHash.GetNext( index ) ) {
		if ( idStr::Icmp( completeStats[ index ].key->c_str(), name ) != 0 ) {
			continue;
		}

		return &completeStats[ index ];
	}

	return NULL;
}
