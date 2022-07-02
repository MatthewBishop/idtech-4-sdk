// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "SDNetManager.h"

#include "../guis/UserInterfaceLocal.h"

#include "../../sdnet/SDNet.h"
#include "../../sdnet/SDNetTask.h"
#include "../../sdnet/SDNetUser.h"
#include "../../sdnet/SDNetAccount.h"
#include "../../sdnet/SDNetProfile.h"
#include "../../sdnet/SDNetSession.h"
#include "../../sdnet/SDNetMessage.h"
#include "../../sdnet/SDNetMessageHistory.h"
#include "../../sdnet/SDNetSessionManager.h"
#include "../../sdnet/SDNetStatsManager.h"
#include "../../sdnet/SDNetFriendsManager.h"
#include "../../sdnet/SDNetTeamManager.h"

#include "../proficiency/StatsTracker.h"
#include "../structures/TeamManager.h"
#include "../rules/GameRules.h"

#include "../guis/UIList.h"

#include "../../framework/Licensee.h"

#include "../../decllib/declTypeHolder.h"

idHashMap< sdNetManager::uiFunction_t* > sdNetManager::uiFunctions;

idCVar net_accountName( "net_accountName", "", CVAR_INIT, "Auto login account name" );
idCVar net_accountPassword( "net_accountPassword", "", CVAR_INIT, "Auto login account password" );
idCVar net_autoConnectServer( "net_autoConnectServer", "", CVAR_INIT, "Server to connect to after auto login is complete" );

/*
================
sdNetManager::sdNetManager
================
*/
sdNetManager::sdNetManager() :
	activeMessage( NULL ),
	refreshServerTask( NULL ),
	findServersTask( NULL ),
	findHistoryServersTask( NULL ),
	findFavoriteServersTask( NULL ),
	findLANServersTask( NULL ),
	gameSession( NULL ),
	lastSessionUpdateTime( -1 ),
	gameTypeNames( NULL ),
	serverRefreshSession( NULL ),
	initFriendsTask( NULL ),
	initTeamsTask( NULL ) {
}

/*
================
sdNetManager::Init
================
*/
void sdNetManager::Init() {
	properties.Init();

	gameTypeNames = gameLocal.declStringMapType.LocalFind( "game/rules/names" );
	offlineString = declHolder.declLocStrType.LocalFind( "guis/mainmenu/offline" );
	infinityString = declHolder.declLocStrType.LocalFind( "guis/mainmenu/infinity" );

	// Initialize functions
	InitFunctions();

	// Connect to the master
	Connect();
}


/*
============
sdNetManager::CancelUserTasks
============
*/
void sdNetManager::CancelUserTasks() {
	if ( findServersTask != NULL ) {
		findServersTask->Cancel( true );
		networkService->FreeTask( findServersTask );
		findServersTask = NULL;
	}

	if ( findLANServersTask != NULL ) {
		findLANServersTask->Cancel( true );
		networkService->FreeTask( findLANServersTask );
		findLANServersTask = NULL;
	}

	if ( refreshServerTask != NULL ) {
		refreshServerTask->Cancel( true );
		networkService->FreeTask( refreshServerTask );
		refreshServerTask = NULL;
	}

	if ( findHistoryServersTask != NULL ) {
		findHistoryServersTask->Cancel( true );
		networkService->FreeTask( findHistoryServersTask );
		findHistoryServersTask = NULL;
	}

	if ( findFavoriteServersTask != NULL ) {
		findFavoriteServersTask->Cancel( true );
		networkService->FreeTask( findFavoriteServersTask );
		findFavoriteServersTask = NULL;
	}

	if ( initTeamsTask != NULL ) {
		initTeamsTask->Cancel( true );
		networkService->FreeTask( initTeamsTask );
		initTeamsTask = NULL;
	}

	if ( initFriendsTask != NULL ) {
		initFriendsTask->Cancel( true );
		networkService->FreeTask( initFriendsTask );
		initFriendsTask = NULL;
	}
}

/*
================
sdNetManager::Connect
================
*/
bool sdNetManager::Connect() {
	assert( networkService->GetState() == sdNetService::SS_INITIALIZED );

	task_t* activeTask = GetTaskSlot();
	if ( activeTask == NULL ) {
		return false;
	}

	return ( activeTask->Set( networkService->Connect(), &sdNetManager::OnConnect ) );
}

/*
================
sdNetManager::OnConnect
================
*/
void sdNetManager::OnConnect( sdNetTask* task, void* parm ) {
	if ( task->GetErrorCode() != SDNET_NO_ERROR ) {
		properties.SetTaskResult( task->GetErrorCode(), declHolder.FindLocStr( va( "sdnet/error/%d", task->GetErrorCode() ) ) );
		properties.ConnectFailed();
	}
}

/*
================
sdNetManager::Shutdown
================
*/
void sdNetManager::Shutdown() {

	// Cancel any outstanding tasks
	for ( int i = 0; i < MAX_ACTIVE_TASKS; i++ ) {
		sdNetTask* activeTask = activeTasks[ i ].task;
		if ( activeTask != NULL ) {
			activeTasks[ i ].Cancel();
			activeTasks[ i ].OnCompleted( this );
			networkService->FreeTask( activeTask );
		}
	}

	if ( activeTask.task != NULL ) {
		activeTask.Cancel();
		activeTask.OnCompleted( this );
		networkService->FreeTask( activeTask.task );
	}

	activeMessage = NULL;

	CancelUserTasks();

	if ( serverRefreshSession != NULL ) {
		networkService->GetSessionManager().FreeSession( serverRefreshSession );
		serverRefreshSession = NULL;
	}

	StopGameSession( true );

	// free outstanding server browser sessions
	for ( int i = 0; i < sessions.Num(); i ++ ) {
		networkService->GetSessionManager().FreeSession( sessions[ i ] );
	}
	sessions.Clear();

	for ( int i = 0; i < sessionsLAN.Num(); i ++ ) {
		networkService->GetSessionManager().FreeSession( sessionsLAN[ i ] );
	}
	sessionsLAN.Clear();

	for ( int i = 0; i < sessionsHistory.Num(); i ++ ) {
		networkService->GetSessionManager().FreeSession( sessionsHistory[ i ] );
	}
	sessionsHistory.Clear();

	for ( int i = 0; i < sessionsFavorites.Num(); i ++ ) {
		networkService->GetSessionManager().FreeSession( sessionsFavorites[ i ] );
	}
	sessionsFavorites.Clear();

	tempWStr.Clear();
	builder.Clear();

	properties.Shutdown();
	ShutdownFunctions();
}

/*
================
sdNetManager::InitFunctions
================
*/

#define ALLOC_FUNC( name, returntype, parms, function ) uiFunctions.Set( name, new uiFunction_t( returntype, parms, function ) )
#pragma inline_depth( 0 )
#pragma optimize( "", off )
void sdNetManager::InitFunctions() {
	ALLOC_FUNC( "connect",					'f', "",		&sdNetManager::Script_Connect );

	ALLOC_FUNC( "validateUsername",			'f', "s",		&sdNetManager::Script_ValidateUsername );
	ALLOC_FUNC( "makeRawUsername",			's', "s",		&sdNetManager::Script_MakeRawUsername );

	ALLOC_FUNC( "validateEmail",			'f', "ss",		&sdNetManager::Script_ValidateEmail );

	ALLOC_FUNC( "createUser",				'v', "s",		&sdNetManager::Script_CreateUser );
	ALLOC_FUNC( "deleteUser",				'v', "s",		&sdNetManager::Script_DeleteUser );
	ALLOC_FUNC( "activateUser",				'f', "s",		&sdNetManager::Script_ActivateUser );
	ALLOC_FUNC( "deactivateUser",			'v', "",		&sdNetManager::Script_DeactivateUser );
	ALLOC_FUNC( "saveUser",					'v', "",		&sdNetManager::Script_SaveUser );
	ALLOC_FUNC( "setDefaultUser",			'v', "sf",		&sdNetManager::Script_SetDefaultUser );	// username, set

#if !defined( SD_DEMO_BUILD )
	ALLOC_FUNC( "createAccount",			'f', "sss",		&sdNetManager::Script_CreateAccount );
	ALLOC_FUNC( "deleteAccount",			'v', "s",		&sdNetManager::Script_DeleteAccount );
	ALLOC_FUNC( "hasAccount",				'f', "",		&sdNetManager::Script_HasAccount );

	ALLOC_FUNC( "accountSetUsername",		'v', "s",		&sdNetManager::Script_AccountSetUsername );
	ALLOC_FUNC( "accountPasswordSet",		'f', "",		&sdNetManager::Script_AccountIsPasswordSet );
	ALLOC_FUNC( "accountSetPassword",		'v', "s",		&sdNetManager::Script_AccountSetPassword );
	ALLOC_FUNC( "accountResetPassword",		'v', "ss",		&sdNetManager::Script_AccountResetPassword );
	ALLOC_FUNC( "accountChangePassword",	'v', "ss",		&sdNetManager::Script_AccountChangePassword );
#endif /* !SD_DEMO_BUILD */

	ALLOC_FUNC( "signIn",					'v', "",		&sdNetManager::Script_SignIn );
	ALLOC_FUNC( "signOut",					'v', "",		&sdNetManager::Script_SignOut );

	ALLOC_FUNC( "getProfileString",			's', "ss",		&sdNetManager::Script_GetProfileString );
	ALLOC_FUNC( "setProfileString",			'v', "ss",		&sdNetManager::Script_SetProfileString );
#if !defined( SD_DEMO_BUILD )
	ALLOC_FUNC( "assureProfileExists",		'v', "",		&sdNetManager::Script_AssureProfileExists );
	ALLOC_FUNC( "storeProfile",				'v', "",		&sdNetManager::Script_StoreProfile );
	ALLOC_FUNC( "restoreProfile",			'v', "",		&sdNetManager::Script_RestoreProfile );
#endif /* !SD_DEMO_BUILD */
	ALLOC_FUNC( "validateProfile",			'f', "",		&sdNetManager::Script_ValidateProfile );

	ALLOC_FUNC( "findServers",				'f', "f",		&sdNetManager::Script_FindServers );				// FS_ source; returns true if success
	ALLOC_FUNC( "addUnfilteredSession",		'v', "s",		&sdNetManager::Script_AddUnfilteredSession );		// IP:Port
	ALLOC_FUNC( "clearUnfilteredSessions",	'v', "",		&sdNetManager::Script_ClearUnfilteredSessions );
	ALLOC_FUNC( "stopFindingServers",		'v', "f",		&sdNetManager::Script_StopFindingServers );			// FS_ source
	ALLOC_FUNC( "refreshCurrentServers",	'f', "f",		&sdNetManager::Script_RefreshCurrentServers );		// FS_ source; returns true if success
	ALLOC_FUNC( "refreshServer",			'f', "ffs",		&sdNetManager::Script_RefreshServer );				// FS_ source, session index, optional session as a string; returns true if success
	ALLOC_FUNC( "joinServer",				'v', "ff",		&sdNetManager::Script_JoinServer );					// FS_ source, session index

#if !defined( SD_DEMO_BUILD )
	ALLOC_FUNC( "checkKey",					'f', "ss",		&sdNetManager::Script_CheckKey );

	ALLOC_FUNC( "initFriends",				'v', "",		&sdNetManager::Script_Friend_Init );
	ALLOC_FUNC( "proposeFriendship",		'v', "sw",		&sdNetManager::Script_Friend_ProposeFriendShip );	// username, reason
	ALLOC_FUNC( "acceptProposal",			'v', "s",		&sdNetManager::Script_Friend_AcceptProposal );
	ALLOC_FUNC( "rejectProposal",			'v', "s",		&sdNetManager::Script_Friend_RejectProposal );
	ALLOC_FUNC( "removeFriend",				'v', "s",		&sdNetManager::Script_Friend_RemoveFriend );
	ALLOC_FUNC( "setBlockedStatus",			'v', "si",		&sdNetManager::Script_Friend_SetBlockedStatus );
	ALLOC_FUNC( "getBlockedStatus",			'f', "s",		&sdNetManager::Script_Friend_GetBlockedStatus );
	ALLOC_FUNC( "sendIM",					'v', "sw",		&sdNetManager::Script_Friend_SendIM );
	ALLOC_FUNC( "getIMText",				'w', "s",		&sdNetManager::Script_Friend_GetIMText );
	ALLOC_FUNC( "getProposalText",			'w', "s",		&sdNetManager::Script_Friend_GetProposalText );
	ALLOC_FUNC( "inviteFriend",				'v', "s",		&sdNetManager::Script_Friend_InviteFriend );
	ALLOC_FUNC( "isFriend",					'f', "s",		&sdNetManager::Script_IsFriend );
	ALLOC_FUNC( "isPendingFriend",			'f', "s",		&sdNetManager::Script_IsPendingFriend );
	ALLOC_FUNC( "isInvitedFriend",			'f', "s",		&sdNetManager::Script_IsInvitedFriend );
	ALLOC_FUNC( "isFriendOnServer",			'f', "s",		&sdNetManager::Script_IsFriendOnServer );
	ALLOC_FUNC( "followFriendToServer",		'f', "s",		&sdNetManager::Script_FollowFriendToServer );
#endif /* !SD_DEMO_BUILD */
	ALLOC_FUNC( "isSelfOnServer",			'f', "",		&sdNetManager::Script_IsSelfOnServer );

#if !defined( SD_DEMO_BUILD )
	ALLOC_FUNC( "loadMessageHistory",		'f', "fs",		&sdNetManager::Script_LoadMessageHistory );		// source, username
	ALLOC_FUNC( "unloadMessageHistory",		'f', "fs",		&sdNetManager::Script_UnloadMessageHistory );	// source, username
	ALLOC_FUNC( "addToMessageHistory",		'f', "fssw",	&sdNetManager::Script_AddToMessageHistory );	// source, username, fromUser, messageFromUser
	ALLOC_FUNC( "getUserNamesForKey",		'f', "s",		&sdNetManager::Script_GetUserNamesForKey );		// key
#endif /* !SD_DEMO_BUILD */

	ALLOC_FUNC( "getPlayerCount",			'f', "f",		&sdNetManager::Script_GetPlayerCount );			// server list source

#if !defined( SD_DEMO_BUILD )
	ALLOC_FUNC( "chooseContextActionForFriend",	'f', "s",	&sdNetManager::Script_Friend_ChooseContextAction );
	ALLOC_FUNC( "numAvailableIMs",				'f', "s",	&sdNetManager::Script_Friend_NumAvailableIMs );
	ALLOC_FUNC( "deleteActiveMessage",		'v', "",		&sdNetManager::Script_DeleteActiveMessage );
	ALLOC_FUNC( "clearActiveMessage",		'v', "",		&sdNetManager::Script_ClearActiveMessage );
#endif /* !SD_DEMO_BUILD */

	ALLOC_FUNC( "queryServerInfo",			's', "ffss",	&sdNetManager::Script_QueryServerInfo );		// server list source, server session, lookup key, default return
	ALLOC_FUNC( "queryMapInfo",				's', "ffss",	&sdNetManager::Script_QueryMapInfo );			// server list source, server session, lookup key, default return
	ALLOC_FUNC( "queryGameType",			'w', "ff",		&sdNetManager::Script_QueryGameType );			// server list source, server session

	ALLOC_FUNC( "requestStats",				'f', "f",		&sdNetManager::Script_RequestStats );
	ALLOC_FUNC( "getStat",					'f', "s",		&sdNetManager::Script_GetStat );
	ALLOC_FUNC( "getPlayerAward",			'w', "ff",		&sdNetManager::Script_GetPlayerAward );			// reward type, player info type (rank, xp, name)
	ALLOC_FUNC( "queryXPStats",				's', "sf",		&sdNetManager::Script_QueryXPStats );			// proficiency category, total (true) or per-map (false)
	ALLOC_FUNC( "queryXPTotals",			's', "f",		&sdNetManager::Script_QueryXPTotals );			// returns total XP, total (true) or per-map (false)

#if !defined( SD_DEMO_BUILD )
	ALLOC_FUNC( "initTeams",				'f', "",		&sdNetManager::Script_Team_Init );

	ALLOC_FUNC( "createTeam",				'v', "s",		&sdNetManager::Script_Team_CreateTeam );
	ALLOC_FUNC( "proposeMembership",		'v', "sw",		&sdNetManager::Script_Team_ProposeMembership );	// username, reason
	ALLOC_FUNC( "acceptMembership",			'f', "s",		&sdNetManager::Script_Team_AcceptMembership );
	ALLOC_FUNC( "rejectMembership",			'f', "s",		&sdNetManager::Script_Team_RejectMembership );
	ALLOC_FUNC( "removeMember",				'v', "s",		&sdNetManager::Script_Team_RemoveMember );
	ALLOC_FUNC( "sendTeamMessage",			'v', "sw",		&sdNetManager::Script_Team_SendMessage );
	ALLOC_FUNC( "broadcastTeamMessage",		'v', "w",		&sdNetManager::Script_Team_BroadcastMessage );
	ALLOC_FUNC( "inviteMember",				'v', "s",		&sdNetManager::Script_Team_Invite );
	ALLOC_FUNC( "promoteMember",			'v', "s",		&sdNetManager::Script_Team_PromoteMember );
	ALLOC_FUNC( "demoteMember",				'v', "s",		&sdNetManager::Script_Team_DemoteMember );
	ALLOC_FUNC( "transferOwnership",		'v', "s",		&sdNetManager::Script_Team_TransferOwnership );
	ALLOC_FUNC( "disbandTeam",				'v', "",		&sdNetManager::Script_Team_DisbandTeam );
	ALLOC_FUNC( "leaveTeam",				'v', "s",		&sdNetManager::Script_Team_LeaveTeam );
	ALLOC_FUNC( "chooseTeamContextAction",	'f', "s",		&sdNetManager::Script_Team_ChooseContextAction );
	ALLOC_FUNC( "isTeamMember",				'f', "s",		&sdNetManager::Script_Team_IsMember );
	ALLOC_FUNC( "isPendingTeamMember",		'f', "s",		&sdNetManager::Script_Team_IsPendingMember );
	ALLOC_FUNC( "getTeamIMText",			'w', "s",		&sdNetManager::Script_Team_GetIMText );
	ALLOC_FUNC( "numAvailableTeamIMs",		'f', "s",		&sdNetManager::Script_Team_NumAvailableIMs );
	ALLOC_FUNC( "getMemberStatus",			'f', "s",		&sdNetManager::Script_Team_GetMemberStatus );
	ALLOC_FUNC( "isMemberOnServer",			'f', "s",		&sdNetManager::Script_IsMemberOnServer );
	ALLOC_FUNC( "followMemberToServer",		'f', "s",		&sdNetManager::Script_FollowMemberToServer );
#endif /* !SD_DEMO_BUILD */

	ALLOC_FUNC( "clearFilters",				'v', "",		&sdNetManager::Script_ClearFilters );
	ALLOC_FUNC( "applyNumericFilter",		'v', "fffff",	&sdNetManager::Script_ApplyNumericFilter );			// type, state, operation, result bin, value
	ALLOC_FUNC( "applyStringFilter",		'v', "sfffs",	&sdNetManager::Script_ApplyStringFilter );			// cvar, state, operation, result bin, value

	ALLOC_FUNC( "saveFilters",				'v', "s",		&sdNetManager::Script_SaveFilters );				// prefix
	ALLOC_FUNC( "loadFilters",				'v', "s",		&sdNetManager::Script_LoadFilters );				// prefix

	ALLOC_FUNC( "queryNumericFilterValue",	'f', "f",		&sdNetManager::Script_QueryNumericFilterValue );	// type
	ALLOC_FUNC( "queryNumericFilterState",	'f', "ff",		&sdNetManager::Script_QueryNumericFilterState );	// type, value
	ALLOC_FUNC( "queryStringFilterState",	'f', "ss",		&sdNetManager::Script_QueryStringFilterState );		// cvar, value
	ALLOC_FUNC( "queryStringFilterValue",	's', "s",		&sdNetManager::Script_QueryStringFilterValue );		// cvar

	ALLOC_FUNC( "joinSession",				'v', "",		&sdNetManager::Script_JoinSession );

	ALLOC_FUNC( "getMotdString",			'w', "",		&sdNetManager::Script_GetMotdString );				// cvar
	ALLOC_FUNC( "getServerStatusString",	'w', "ff",		&sdNetManager::Script_GetServerStatusString );		// server list source, server session

	ALLOC_FUNC( "toggleServerFavoriteState",'v', "s",		&sdNetManager::Script_ToggleServerFavoriteState );	// session IP:Port

#if !defined( SD_DEMO_BUILD )
	ALLOC_FUNC( "getFriendServerIP",		's', "s",		&sdNetManager::Script_GetFriendServerIP );			// friend name, returns "IP:Port" or ""
	ALLOC_FUNC( "getMemberServerIP",		's', "s",		&sdNetManager::Script_GetMemberServerIP );			// member name, returns "IP:Port" or ""
	ALLOC_FUNC( "getMessageTimeStamp",		'w', "",		&sdNetManager::Script_GetMessageTimeStamp );		// timestamp string
	ALLOC_FUNC( "getServerInviteIP",		's', "",		&sdNetManager::Script_GetServerInviteIP );
#endif /* !SD_DEMO_BUILD */

	ALLOC_FUNC( "cancelActiveTask",			'v', "",		&sdNetManager::Script_CancelActiveTask );

	ALLOC_FUNC( "formatSessionInfo",		'w', "s",		&sdNetManager::Script_FormatSessionInfo );			// IP:Port
}

#pragma optimize( "", on )
#pragma inline_depth()

#undef ALLOC_FUNC

/*
================
sdNetManager::ShutdownFunctions
================
*/
void sdNetManager::ShutdownFunctions() {
	uiFunctions.DeleteContents();
}

/*
================
sdNetManager::FindFunction
================
*/
sdNetManager::uiFunction_t* sdNetManager::FindFunction( const char* name ) {
	uiFunction_t** ptr;
	return uiFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdNetManager::GetFunction
============
*/
sdUIFunctionInstance* sdNetManager::GetFunction( const char* name ) {
	uiFunction_t* function = FindFunction( name );
	if ( function == NULL ) {
		return NULL;
	}

	return new sdUITemplateFunctionInstance< sdNetManager, sdUITemplateFunctionInstance_Identifier >( this, function );
}

/*
================
sdNetManager::RunFrame
================
*/
void sdNetManager::RunFrame() {
	properties.UpdateProperties();

	// task processing

	// process parallel tasks
	for ( int i = 0; i < MAX_ACTIVE_TASKS; i++ ) {
		sdNetTask* activeTask = activeTasks[ i ].task;

		if ( activeTask != NULL && activeTask->GetState() == sdNetTask::TS_DONE ) {
			activeTasks[ i ].OnCompleted( this );
			networkService->FreeTask( activeTask );
		}
	}

	// process single 'serial' task (these are tasks usually started by the player from a gui)
	if ( activeTask.task != NULL ) {
		if ( activeTask.task->GetState() == sdNetTask::TS_DONE ) {
			sdNetTask* completedTask = activeTask.task;

			activeTask.OnCompleted( this );

			properties.SetTaskResult( completedTask->GetErrorCode(), declHolder.FindLocStr( va( "sdnet/error/%d", completedTask->GetErrorCode() ) ) );
			properties.SetTaskActive( false );

			networkService->FreeTask( completedTask );
		}
	}

	if ( findServersTask ) {
		properties.SetNumAvailableDWServers( sessions.Num() );

		if ( findServersTask->GetState() == sdNetTask::TS_DONE ) {
			networkService->FreeTask( findServersTask );
			findServersTask = NULL;
		}
	}

	if ( findLANServersTask ) {
		properties.SetNumAvailableLANServers( sessionsLAN.Num() );

		if ( findLANServersTask->GetState() == sdNetTask::TS_DONE ) {
			networkService->FreeTask( findLANServersTask );
			findLANServersTask = NULL;
		}
	}

	if ( findHistoryServersTask ) {
		properties.SetNumAvailableHistoryServers( sessionsHistory.Num() );

		if ( findHistoryServersTask->GetState() == sdNetTask::TS_DONE ) {
			networkService->FreeTask( findHistoryServersTask );
			findHistoryServersTask = NULL;
		}
	}

	if ( findFavoriteServersTask ) {
		properties.SetNumAvailableFavoritesServers( sessionsFavorites.Num() );

		if ( findFavoriteServersTask->GetState() == sdNetTask::TS_DONE ) {
			networkService->FreeTask( findFavoriteServersTask );
			findFavoriteServersTask = NULL;
		}
	}

#if !defined( SD_DEMO_BUILD )
	bool findingServers = findHistoryServersTask != NULL || findServersTask != NULL || findLANServersTask != NULL || findFavoriteServersTask != NULL;
	properties.SetFindingServers( findingServers );
	if( !findingServers ) {
		lastServerUpdateIndex = 0;
	}

	if ( initFriendsTask && initFriendsTask->GetState() == sdNetTask::TS_DONE ) {
		networkService->FreeTask( initFriendsTask );
		initFriendsTask = NULL;
	}

	if ( initTeamsTask && initTeamsTask->GetState() == sdNetTask::TS_DONE ) {
		networkService->FreeTask( initTeamsTask );
		initTeamsTask = NULL;
	}

	properties.SetInitializingFriends( initFriendsTask != NULL );
	properties.SetInitializingTeams( initTeamsTask != NULL );
#endif /* !SD_DEMO_BUILD */

	if( refreshServerTask ) {
		if( refreshServerTask->GetState() == sdNetTask::TS_DONE ) {
			networkService->FreeTask( refreshServerTask );
			refreshServerTask = NULL;

			if( serverRefreshSession != NULL ) {
				sdNetTask* task;
				idList< sdNetSession* >* netSessions;
				GetSessionsForServerSource( serverRefreshSource, netSessions, task );

				// replace the existing session with the updated one
				if( netSessions != NULL ) {
					sessionHash_t::Iterator iter = hashedSessions.Find( serverRefreshSession->GetHostAddressString() );
					if( iter != hashedSessions.End() ) {
						if( task != NULL ) {
							task->AcquireLock();
						}

						int index = iter->second.sessionListIndex;
						if( index >= 0 && index < netSessions->Num() ) {
							const char* newIP = serverRefreshSession->GetHostAddressString();
							const char* oldIP = (*netSessions)[ index ]->GetHostAddressString();
							if( idStr::Cmp( newIP, oldIP ) == 0 ) {
								networkService->GetSessionManager().FreeSession( (*netSessions)[ index ] );
								(*netSessions)[ index ] = serverRefreshSession;
								serverRefreshSession = NULL;
							} else {
								assert( false );
							}
						}

						if( task != NULL ) {
							task->ReleaseLock();
						}
					}
				} else {
					assert( false );
				}
			}

			properties.SetServerRefreshComplete( true );
		}
	}

#if !defined( SD_DEMO_BUILD )
	// FIXME: don't update this every frame
	const sdNetTeamManager& manager = networkService->GetTeamManager();
	properties.SetTeamName( manager.GetTeamName() );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& pending = networkService->GetTeamManager().GetPendingInvitesList();
	properties.SetNumPendingClanInvites( pending.Num() );
	properties.SetTeamMemberStatus( manager.GetMemberStatus() );
#endif /* !SD_DEMO_BUILD */
}

/*
============
sdNetManager::GetSessionsForServerSource
============
*/
void sdNetManager::GetSessionsForServerSource( findServerSource_e source, idList< sdNetSession* >*& netSessions, sdNetTask*& task ) {
	switch( source ) {
		case FS_INTERNET:
			task = findServersTask;
			netSessions = &sessions;
			break;
		case FS_LAN:
			task = findLANServersTask;
			netSessions = &sessionsLAN;
			break;
		case FS_HISTORY:
			task = findHistoryServersTask;
			netSessions = &sessionsHistory;
			break;
		case FS_FAVORITES:
			task = findFavoriteServersTask;
			netSessions = &sessionsFavorites;
			break;
	}
}


/*
============
sdNetManager::GetGameType
============
*/
void sdNetManager::GetGameType( const char* siRules, idWStr& type ) {
	if( siRules[ 0 ] == '\0' ) {
		type = L"";
		return;
	}
	const char* gameTypeLoc = gameTypeNames->GetDict().GetString( siRules, "" );

	if( gameTypeLoc[ 0 ] != '\0' ) {
		type = common->LocalizeText( gameTypeLoc );
	} else {
		type= va( L"%hs", siRules );
	}
}

/*
================
sdNetManager::CreateServerList
================
*/
void sdNetManager::CreateServerList( sdUIList* list, findServerSource_e source, findServerMode_e mode, bool flagOffline ) {
	if( flagOffline ) {
		for( int i = 0; i < list->GetNumItems(); i++ ) {
			if( list->GetItemDataInt( i, 0 ) == -1 ) {
				sdUIList::SetItemText( list, va( L"(%ls) %ls", offlineString->GetText(), list->GetItemText( i, 0, true ) ), i, 4 );
			}
		}
		return;
	}

	sdNetTask* task = NULL;
	idList< sdNetSession* >* netSessions = NULL;

	GetSessionsForServerSource( source, netSessions, task );

	if( netSessions == NULL ) {
		assert( 0 );
		return;
	}

	if ( task != NULL ) {
		task->AcquireLock();
	}

	list->SetItemGranularity( 1024 );
	list->BeginBatch();

	if( lastServerUpdateIndex == 0 ) {
		if ( mode == FSM_NEW ) {
			hashedSessions.Clear();
			hashedSessions.SetGranularity( 1024 );
			sdUIList::ClearItems( list );
		} else if( mode == FSM_REFRESH ) {
			for( int i = 0; i < list->GetNumItems(); i++ ) {
				list->SetItemDataInt( -1, i, 0 );		// flag all items as not updated so servers that have dropped won't show bad info
			}
		}
	}

	if ( mode == FSM_NEW ) {
#if !defined( SD_DEMO_BUILD )
		bool ranked = ShowRanked();
#else
		bool ranked = false;
#endif /* !SD_DEMO_BUILD */
		idStr address;

		for ( int i = lastServerUpdateIndex; i < netSessions->Num(); i++ ) {
			sdNetSession* netSession = (*netSessions)[ i ];
			
			if( DoFiltering( *netSession ) ) {
				if( ranked && !netSession->IsRanked() ) {
					continue;
				}
				if ( SessionIsFiltered( *netSession ) ) {
					continue;
				}
			}

			address = netSession->GetHostAddressString();
			assert( !address.IsEmpty() );

			int index = sdUIList::InsertItem( list, va( L"%hs", address.c_str() ), -1, BC_IP );

			assert( hashedSessions.Find( address.c_str() ) == hashedSessions.End() );
			assert( hashedSessions.Num() == list->GetNumItems() - 1 );

			sessionIndices_t& info = hashedSessions[ address.c_str() ];
			info.sessionListIndex = i;
			info.uiListIndex = index;

			UpdateSession( *list, *netSession, index );
			list->SetItemDataInt( i, index, 0, true );		// store the session index

		}
	} else if ( mode == FSM_REFRESH ) {
		for ( int i = lastServerUpdateIndex; i < netSessions->Num(); i++ ) {
			sdNetSession* netSession = (*netSessions)[ i ];
			const idDict& serverInfo = netSession->GetServerInfo();

			sessionHash_t::Iterator iter = hashedSessions.Find( netSession->GetHostAddressString() );
			if( iter == hashedSessions.End() ) {
				continue;
			} else {
				UpdateSession( *list, *netSession, iter->second.uiListIndex );
				list->SetItemDataInt( i, iter->second.uiListIndex, BC_IP, true );		// store the session index
				iter->second.sessionListIndex = i;
				assert( idWStr::Icmp( list->GetItemText( iter->second.uiListIndex, 0 ), va( L"%hs", netSession->GetHostAddressString() ) ) == 0 );
			}
		}
	}

	lastServerUpdateIndex = netSessions->Num();
	assert( hashedSessions.Num() == list->GetNumItems() );

	if ( task != NULL ) {
		task->ReleaseLock();
	}
	list->EndBatch();
}

/*
============
sdNetManager::UpdateSession
============
*/
void sdNetManager::UpdateSession( sdUIList& list, sdNetSession& netSession, int index ) {
	sdNetUser* activeUser = networkService->GetActiveUser();
	bool favorite = activeUser->GetProfile().GetProperties().GetBool( va( "favorite_%s", netSession.GetHostAddressString() ), "0" );

	// Favorite state
	sdUIList::SetItemText( &list, favorite ? L"<material = 'favorite_set'>f" : L"<material = 'favorite_unset'>", index, BC_FAVORITE );

	if ( netSession.GetPing() == 999 ) {
		// Password
		sdUIList::SetItemText( &list, L"", index, BC_PASSWORD );

		// Ranked
		sdUIList::SetItemText( &list, L"", index, BC_RANKED );

		// Server name
		sdUIList::SetItemText( &list, va( L"(%ls) %hs", offlineString->GetText(), netSession.GetHostAddressString() ), index, BC_NAME );

		// Map name
		sdUIList::SetItemText( &list, L"", index, BC_MAP );

		// Game Type Icon
		sdUIList::SetItemText( &list, L"", index, BC_GAMETYPE_ICON );

		// Game Type
		sdUIList::SetItemText( &list, L"", index, BC_GAMETYPE );

		// Time left
		list.SetItemDataInt( 0, index, BC_TIMELEFT, true );
		sdUIList::SetItemText( &list, L"", index, BC_TIMELEFT );

		// Client Count
		list.SetItemDataInt( 0, index, BC_PLAYERS, true );
		sdUIList::SetItemText( &list, L"", index, BC_PLAYERS );

		// Ping
		list.SetItemDataInt( 0, index, BC_PING, true );
		sdUIList::SetItemText( &list, L"-1", index, BC_PING );
	} else {
		const idDict& serverInfo = netSession.GetServerInfo();
		const idDict* mapInfo = GetMapInfo( netSession );

		// Password
		sdUIList::SetItemText( &list, va( L"%hs", serverInfo.GetBool( "si_needPass", "0" ) ? "<material = 'password'>p" : "" ), index, BC_PASSWORD);

		// Ranked
		sdUIList::SetItemText( &list, va( L"%hs", netSession.IsRanked() ? "<material = 'ranked'>r" : "" ), index, BC_RANKED );

		tempWStr = L"";
		const char* serverName = serverInfo.GetString( "si_name" );
		if( serverName[ 0 ] == '\0' ) {
			tempWStr = va( L"(%ls) %hs", offlineString->GetText(), netSession.GetHostAddressString() );
		} else {
			tempWStr = va( L"%hs", serverName );
			sdUIList::CleanUserInput( tempWStr );
		}

		// strip leading spaces
		const wchar_t* nameStr = tempWStr.c_str();
		while( *nameStr != L'\0' && ( *nameStr == L' ' || *nameStr == L'\t' ) ) {
			nameStr++;
		}


		// Server name
		sdUIList::SetItemText( &list, nameStr, index, BC_NAME );

		// Map name
		if ( mapInfo == NULL ) {
			tempWStr = va( L"%hs", serverInfo.GetString( "si_map" ) );
			tempWStr.StripFileExtension();

			sdUIList::CleanUserInput( tempWStr );

			// skip the path
			int offset = tempWStr.Length() - 1;
			if( offset >= 0 ) {
				while( offset >= 0 ) {
					if( tempWStr[ offset ] == L'/' || tempWStr[ offset ] == L'\\' ) {
						offset++;
						break;
					}
					offset--;
				}
				// jrad - the case of "path/" should never happen, but let's play it safe
				if( offset >= tempWStr.Length() ) {
					offset = 0;
				}

				sdUIList::SetItemText( &list, &tempWStr[ offset ], index, BC_MAP );
			} else {
				sdUIList::SetItemText( &list, L"", index, BC_MAP );
			}
		} else {
			tempWStr = mapInfo != NULL ? va( L"%hs", mapInfo->GetString( "pretty_name", serverInfo.GetString( "si_map" ) ) ) : L"";
			tempWStr.StripFileExtension();
			sdUIList::CleanUserInput( tempWStr );
			sdUIList::SetItemText( &list, tempWStr.c_str(), index, BC_MAP );
		}


		// Game Type
		const char* fsGame = serverInfo.GetString( "fs_game" );

		if( !*fsGame ) {			
			const char* siRules = serverInfo.GetString( "si_rules" );
			GetGameType( siRules, tempWStr );

			const char* mat = list.GetUI()->GetMaterial( siRules );
			if( *mat ) {
				sdUIList::SetItemText( &list, va( L"<material = '%hs'>", siRules ), index, BC_GAMETYPE_ICON );
			} else {
				sdUIList::SetItemText( &list, L"", index, BC_GAMETYPE_ICON );
			}
		} else {
			sdUIList::SetItemText( &list, L"<material = '_unknownGameType'>", index, BC_GAMETYPE_ICON );
			tempWStr = va( L"%hs", fsGame );
		}		

		sdUIList::SetItemText( &list, tempWStr.c_str(), index, BC_GAMETYPE );

		// Time left
		if( netSession.GetGameState() & sdGameRules::PGS_WARMUP ) {
			sdUIList::SetItemText( &list, L"<loc = 'guis/mainmenu/server/warmup'>", index, BC_TIMELEFT );
			list.SetItemDataInt( 0, index, BC_TIMELEFT, true );
		} else if( netSession.GetGameState() & sdGameRules::PGS_REVIEWING ) {
			sdUIList::SetItemText( &list, L"<loc = 'guis/mainmenu/server/reviewing'>", index, BC_TIMELEFT );
			list.SetItemDataInt( 0, index, BC_TIMELEFT, true );
		} else if( netSession.GetGameState() & sdGameRules::PGS_LOADING ) {
			sdUIList::SetItemText( &list, L"<loc = 'guis/mainmenu/server/loading'>", index, BC_TIMELEFT );
			list.SetItemDataInt( 0, index, BC_TIMELEFT, true );
		} else {
			if( netSession.GetSessionTime() == 0 ) {
				sdUIList::SetItemText( &list, infinityString->GetText(), index, BC_TIMELEFT );
			} else {
				idWStr::hmsFormat_t format;
				format.showZeroSeconds = false;
				format.showZeroMinutes = true;
				sdUIList::SetItemText( &list, idWStr::MS2HMS( netSession.GetSessionTime(), format ), index, BC_TIMELEFT );
			}
			list.SetItemDataInt( netSession.GetSessionTime(), index, BC_TIMELEFT, true );
		}

		// Client Count
		int numBots = netSession.GetNumBotClients();
		if( numBots == 0 ) {
			sdUIList::SetItemText( &list, va( L"%d/%d", netSession.GetNumClients(), serverInfo.GetInt( "si_maxPlayers" ) ), index, BC_PLAYERS );
		} else {
			sdUIList::SetItemText( &list, va( L"%d/%d (%d)", netSession.GetNumClients(), serverInfo.GetInt( "si_maxPlayers" ), numBots ), index, BC_PLAYERS );
		}

		list.SetItemDataInt( netSession.GetNumClients(), index, BC_PLAYERS, true );	// store the number of clients for proper numeric sorting

		// Ping
		list.SetItemDataInt( netSession.GetPing(), index, BC_PING, true );
		sdUIList::SetItemText( &list, va( L"%d", netSession.GetPing() ), index, BC_PING );
	}
}

/*
================
sdNetManager::SignInDedicated
================
*/
bool sdNetManager::SignInDedicated() {
	assert( networkService->GetDedicatedServerState() == sdNetService::DS_OFFLINE );

	task_t* activeTask = GetTaskSlot();
	if ( activeTask == NULL ) {
		return false;
	}

	return ( activeTask->Set( networkService->SignInDedicated(), &sdNetManager::OnSignInDedicated ) );
}

/*
================
sdNetManager::OnSignInDedicated
================
*/
void sdNetManager::OnSignInDedicated( sdNetTask* task, void* parm ) {
	if ( task->GetErrorCode() != SDNET_NO_ERROR ) {
		// FIXME: error out?
	}
}

/*
================
sdNetManager::SignOutDedicated
================
*/
bool sdNetManager::SignOutDedicated() {
	if ( networkService->GetDedicatedServerState() != sdNetService::DS_ONLINE ) {
		return false;
	}

	task_t* activeTask = GetTaskSlot();
	if ( activeTask == NULL ) {
		return false;
	}

	return ( activeTask->Set( networkService->SignOutDedicated() ) );
}

/*
================
sdNetManager::StartGameSession
================
*/
bool sdNetManager::StartGameSession() {
	if ( gameSession != NULL ) {
		return false;
	}

	if ( !NeedsGameSession() ) {
		// only advertise dedicated non-LAN games
		return false;
	}

	task_t* activeTask = GetTaskSlot();
	if ( activeTask == NULL ) {
		return false;
	}

	gameSession = networkService->GetSessionManager().AllocSession();

	gameSession->GetServerInfo() = gameLocal.serverInfo;

	if ( activeTask->Set( networkService->GetSessionManager().CreateSession( *gameSession ), &sdNetManager::OnGameSessionCreated ) ) {
		lastSessionUpdateTime = gameLocal.time;
		return true;
	}

	networkService->GetSessionManager().FreeSession( gameSession );
	gameSession = NULL;

	return false;
}

/*
================
sdNetManager::UpdateGameSession
================
*/
bool sdNetManager::UpdateGameSession( bool checkDict, bool throttle ) {
	if ( gameSession == NULL ) {
		return false;
	}

	if ( gameSession->GetState() != sdNetSession::SS_IDLE && throttle ) {
		if ( lastSessionUpdateTime + SESSION_UPDATE_INTERVAL > gameLocal.time ) {
			return false;
		}
	}

	if ( checkDict ) {
		unsigned int sessionChecksum = gameSession->GetServerInfo().Checksum();
		unsigned int gameCheckSum = gameLocal.serverInfo.Checksum();

		if ( sessionChecksum == gameCheckSum ) {
			return false;
		}
	}

	task_t* activeTask = GetTaskSlot();
	if ( activeTask == NULL ) {
		return false;
	}

	gameSession->GetServerInfo() = gameLocal.serverInfo;

	lastSessionUpdateTime = gameLocal.time;

	if ( gameSession->GetState() == sdNetSession::SS_IDLE ) {
		// TODO: document in which case this can happen
		// One of the cases is when the dedicated server disconnects and
		// reconnects. We still have a game session, but need to advertise
		// it all over again.
		return ( activeTask->Set( networkService->GetSessionManager().CreateSession( *gameSession ), &sdNetManager::OnGameSessionCreated ) );
	} else if ( gameSession->GetState() == sdNetSession::SS_ADVERTISED ) {
		return ( activeTask->Set( networkService->GetSessionManager().UpdateSession( *gameSession ) ) );
	} else {
		return false;
	}
}

/*
================
sdNetManager::OnGameSessionCreated
================
*/
void sdNetManager::OnGameSessionCreated( sdNetTask* task, void* parm ) {
	if ( task->GetErrorCode() == SDNET_NO_ERROR ) {
		SendSessionId();

		// connect all clients to the server
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			sdNetClientId netClientId;

			networkSystem->ServerGetClientNetId( i, netClientId );

			if ( netClientId.IsValid() ) {
				gameSession->ServerClientConnect( i );
			}
		}
	}
}

/*
================
sdNetManager::StopGameSession
================
*/
bool sdNetManager::StopGameSession( bool blocking ) {
	if ( gameSession == NULL ) {
		return false;
	}

	if ( gameSession->GetState() == sdNetSession::SS_IDLE ) {
		networkService->GetSessionManager().FreeSession( gameSession );
		gameSession = NULL;
		return true;
	}

	if ( blocking ) {
		sdNetTask* task = networkService->GetSessionManager().DeleteSession( *gameSession );

		if ( task != NULL ) {
			while ( task->GetState() != sdNetTask::TS_COMPLETING );

			networkService->FreeTask( task );
			networkService->GetSessionManager().FreeSession( gameSession );
			gameSession = NULL;
		}

		return true;
	} else {
		task_t* activeTask = GetTaskSlot();
		if ( activeTask == NULL ) {
			return false;
		}

		return ( activeTask->Set( networkService->GetSessionManager().DeleteSession( *gameSession ), &sdNetManager::OnGameSessionStopped ) );
	}
}

/*
================
sdNetManager::OnGameSessionStopped
================
*/
void sdNetManager::OnGameSessionStopped( sdNetTask* task, void* parm ) {
	if ( task->GetErrorCode() == SDNET_NO_ERROR ) {
		networkService->GetSessionManager().FreeSession( gameSession );
		gameSession = NULL;
	}
}

/*
================
sdNetManager::ServerClientConnect
================
*/
void sdNetManager::ServerClientConnect( const int clientNum ) {
	if ( gameSession == NULL ) {
		return;
	}

	gameSession->ServerClientConnect( clientNum );
}

/*
================
sdNetManager::ServerClientDisconnect
================
*/
void sdNetManager::ServerClientDisconnect( const int clientNum ) {
	if ( gameSession == NULL ) {
		return;
	}

	gameSession->ServerClientDisconnect( clientNum );
}

/*
================
sdNetManager::SendSessionId
	clientNum == -1 : broadcast to all clients
================
*/
void sdNetManager::SendSessionId( const int clientNum ) const {
	if ( !gameLocal.isServer || gameSession == NULL ) {
		return;
	}

	sdNetSession::sessionId_t sessionId;

	gameSession->GetId( sessionId );

	sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_SESSION_ID );

	for ( int i = 0; i < sizeof( sessionId.id ) / sizeof( sessionId.id[0] ); i++ ) {
		msg.WriteByte( sessionId.id[i] );
	}

	msg.Send( clientNum );
}

/*
================
sdNetManager::ProcessSessionId
================
*/
void sdNetManager::ProcessSessionId( const idBitMsg& msg ) {
	for ( int i = 0; i < sizeof( sessionId.id ) / sizeof( sessionId.id[0] ); i++ ) {
		sessionId.id[i] = msg.ReadByte();
	}
}

#if !defined( SD_DEMO_BUILD )
/*
================
sdNetManager::FlushStats
================
*/
void sdNetManager::FlushStats( bool blocking ) {
	if ( networkService->GetDedicatedServerState() != sdNetService::DS_ONLINE ) {
		return;
	}

	if ( blocking ) {
		sdNetTask* task = networkService->GetStatsManager().Flush();

		if ( task != NULL ) {
			sdSignal waitSignal;
			int i = 0;

			common->SetRefreshOnPrint( true );
			while ( task->GetState() != sdNetTask::TS_COMPLETING ) {
				const char spinner[4] = { '|', '/', '-', '\\' };
				gameLocal.Printf( "\rWriting pending stats... [ %c ]", spinner[ i % 4 ] );
				waitSignal.Wait( 33 );
				sys->ProcessOSEvents();
				i++;
			}
			networkService->FreeTask( task );
			gameLocal.Printf( "\rWriting pending stats... [ done ]\n" );
			common->SetRefreshOnPrint( false );
		}
	} else {
		task_t* activeTask = GetTaskSlot();
		if ( activeTask == NULL ) {
			return;
		}

		activeTask->allowCancel = false;
		activeTask->Set( networkService->GetStatsManager().Flush() );
	}
}
#endif /* !SD_DEMO_BUILD */

/*
================
sdNetManager::FindUser
================
*/
sdNetUser* sdNetManager::FindUser( const char* username ) {
	for ( int i = 0; i < networkService->NumUsers(); i++ ) {
		sdNetUser* user = networkService->GetUser( i );

		if ( !idStr::Cmp( username, user->GetRawUsername() ) ) {
			return user;
		}
	}

	return NULL;
}

/*
================
sdNetManager::Script_Connect
================
*/
void sdNetManager::Script_Connect( sdUIFunctionStack& stack ) {
	stack.Push( Connect() ? 1.0f : 0.0f );
}

/*
================
sdNetManager::Script_ValidateUsername
================
*/
void sdNetManager::Script_ValidateUsername( sdUIFunctionStack& stack ) {
	idStr username;
	stack.Pop( username );

	if ( username.IsEmpty() ) {
		stack.Push( sdNetProperties::UV_EMPTY_NAME );
		return;
	}

	if ( FindUser( username.c_str() ) != NULL ) {
		stack.Push( sdNetProperties::UV_DUPLICATE_NAME );
		return;
	}

	for( int i = 0; i < username.Length(); i++ ) {
		if( username[ i ] == '.' || username[ i ] == '_' || username[ i ] == '-' ) {
			continue;
		}
		if( username[ i ] >= 'a' && username[ i ] <= 'z' ) {
			continue;
		}
		if( username[ i ] >= 'A' && username[ i ] <= 'Z' ) {
			continue;
		}
		if( username[ i ] >= '0' && username[ i ] <= '9' ) {
			continue;
		}
		stack.Push( sdNetProperties::UV_INVALID_NAME );
		return;
	}

	stack.Push( sdNetProperties::UV_VALID );
}

/*
================
sdNetManager::Script_MakeRawUsername
================
*/
void sdNetManager::Script_MakeRawUsername( sdUIFunctionStack& stack ) {
	idStr username;
	stack.Pop( username );

	sdNetUser::MakeRawUsername( username.c_str(), username );

	stack.Push( username.c_str() );
}

/*
================
sdNetManager::Script_ValidateEmail
================
*/
void sdNetManager::Script_ValidateEmail( sdUIFunctionStack& stack ) {
	idStr email;
	idStr confirmEmail;

	stack.Pop( email );
	stack.Pop( confirmEmail );

	if ( email.IsEmpty() || confirmEmail.IsEmpty() ) {
		stack.Push( sdNetProperties::EV_EMPTY_MAIL );
		return;
	}

	if ( !email.IsValidEmailAddress() ) {
		stack.Push( sdNetProperties::EV_INVALID_MAIL );
		return;
	}

	if ( email.Cmp( confirmEmail.c_str() ) != 0 ) {
		stack.Push( sdNetProperties::EV_CONFIRM_MISMATCH );
		return;
	}

	stack.Push( sdNetProperties::EV_VALID );
}

/*
================
sdNetManager::Script_CreateUser
================
*/
void sdNetManager::Script_CreateUser( sdUIFunctionStack& stack ) {
	idStr username;
	stack.Pop( username );

	sdNetUser* user;

	networkService->CreateUser( &user, username.c_str() );
}

/*
================
sdNetManager::Script_DeleteUser
================
*/
void sdNetManager::Script_DeleteUser( sdUIFunctionStack& stack ) {
	idStr username;
	stack.Pop( username );

	sdNetUser* user = FindUser( username.c_str() );
	if ( user != NULL ) {
		assert( networkService->GetActiveUser() != user );

		networkService->DeleteUser( user );
	}
}

/*
================
sdNetManager::Script_ActivateUser
================
*/
void sdNetManager::Script_ActivateUser( sdUIFunctionStack& stack ) {
	idStr username;
	stack.Pop( username );
	username.ToLower();

	assert( networkService->GetActiveUser() == NULL );

	sdNetUser* user = FindUser( username.c_str() );
	if ( user != NULL ) {
		user->Activate();
	}
	stack.Push( user != NULL );
}

/*
================
sdNetManager::Script_DeactivateUser
================
*/
void sdNetManager::Script_DeactivateUser( sdUIFunctionStack& stack ) {
	if( networkService->GetActiveUser() == NULL ) {
		assert( networkService->GetActiveUser() != NULL );
		gameLocal.Warning( "Script_DeactivateUser: no active user" );
		return;
	}

	networkService->GetActiveUser()->Deactivate();
}

/*
================
sdNetManager::Script_SaveUser
================
*/
void sdNetManager::Script_SaveUser( sdUIFunctionStack& stack ) {
	if( networkService->GetActiveUser() == NULL ) {
		assert( networkService->GetActiveUser() != NULL );
		gameLocal.Warning( "Script_SaveUser: no active user" );
		return;
	}

	networkService->GetActiveUser()->Save();
}

#if !defined( SD_DEMO_BUILD )
/*
================
sdNetManager::Script_CreateAccount
================
*/
void sdNetManager::Script_CreateAccount( sdUIFunctionStack& stack ) {
	idStr username, password, keyCode;
	stack.Pop( username );
	stack.Pop( password );
	stack.Pop( keyCode );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::CreateAccount : task pending\n" );
		stack.Push( 0.0f );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	if ( activeUser == NULL ) {
		gameLocal.Printf( "SDNet::CreateAccount : no active user\n" );
		stack.Push( 0.0f );
		return;
	}

	if ( !activeTask.Set( activeUser->GetAccount().CreateAccount( username, password, keyCode ) ) ) {
		gameLocal.Printf( "SDNet::CreateAccount : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
		stack.Push( 0.0f );
		return;
	}

	properties.SetTaskActive( true );
	stack.Push( 1.0f );
}

/*
================
sdNetManager::Script_DeleteAccount
================
*/
void sdNetManager::Script_DeleteAccount( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr password;
	stack.Pop( password );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::DeleteAccount : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	activeUser->GetAccount().SetPassword( password.c_str() );

	if ( !activeTask.Set( activeUser->GetAccount().DeleteAccount() ) ) {
		gameLocal.Printf( "SDNet::DeleteAccount : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
================
sdNetManager::Script_HasAccount
================
*/
void sdNetManager::Script_HasAccount( sdUIFunctionStack& stack ) {
	if( networkService->GetActiveUser() == NULL ) {
		stack.Push( 0.0f );
		return;
	}

	stack.Push( *(networkService->GetActiveUser()->GetAccount().GetUsername()) == '\0' ? 0.0f : 1.0f );
}

/*
================
sdNetManager::Script_AccountSetPassword
================
*/
void sdNetManager::Script_AccountSetUsername( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr username;
	stack.Pop( username );

	networkService->GetActiveUser()->GetAccount().SetUsername( username.c_str() );
}

/*
================
sdNetManager::Script_AccountIsPasswordSet
================
*/
void sdNetManager::Script_AccountIsPasswordSet( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	stack.Push( *(networkService->GetActiveUser()->GetAccount().GetPassword()) == '\0' ? 0.0f : 1.0f );
}

/*
================
sdNetManager::Script_AccountSetPassword
================
*/
void sdNetManager::Script_AccountSetPassword( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr password;
	stack.Pop( password );

	networkService->GetActiveUser()->GetAccount().SetPassword( password.c_str() );
}

/*
================
sdNetManager::Script_AccountResetPassword
================
*/
void sdNetManager::Script_AccountResetPassword( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr key;
	stack.Pop( key );

	idStr password;
	stack.Pop( password );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::AccountResetPassword : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetActiveUser()->GetAccount().ResetPassword( key.c_str(), password.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::AccountResetPassword : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
================
sdNetManager::Script_AccountChangePassword
================
*/
void sdNetManager::Script_AccountChangePassword( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr password;
	idStr newPassword;
	stack.Pop( password );
	stack.Pop( newPassword );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::AccountChangePassword : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	if ( !activeTask.Set( activeUser->GetAccount().ChangePassword( password.c_str(), newPassword.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::AccountChangePassword : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}
#endif /* !SD_DEMO_BUILD */

/*
================
sdNetManager::Script_SignIn
================
*/
void sdNetManager::Script_SignIn( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );
	assert( networkService->GetActiveUser()->GetState() == sdNetUser::US_ACTIVE );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::SignIn : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	if ( !activeTask.Set( activeUser->GetAccount().SignIn() ) ) {
		gameLocal.Printf( "SDNet::SignIn : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
================
sdNetManager::Script_SignOut
================
*/
void sdNetManager::Script_SignOut( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );
	assert( networkService->GetActiveUser()->GetState() == sdNetUser::US_ONLINE );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::SignOut : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	CancelUserTasks();

	if ( !activeTask.Set( activeUser->GetAccount().SignOut() ) ) {
		gameLocal.Printf( "SDNet::SignOut : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
================
sdNetManager::Script_GetProfileString
================
*/
void sdNetManager::Script_GetProfileString( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr key;
	stack.Pop( key );

	idStr defaultValue;
	stack.Pop( defaultValue );

	sdNetUser* activeUser = networkService->GetActiveUser();

	stack.Push( activeUser->GetProfile().GetProperties().GetString( key.c_str(), defaultValue.c_str() ) );
}

/*
================
sdNetManager::Script_SetProfileString
================
*/
void sdNetManager::Script_SetProfileString( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr key, value;
	stack.Pop( key );
	stack.Pop( value );

	sdNetUser* activeUser = networkService->GetActiveUser();

	activeUser->GetProfile().GetProperties().Set( key.c_str(), value.c_str() );
}

#if !defined( SD_DEMO_BUILD )
/*
================
sdNetManager::Script_AssureProfileExists
================
*/
void sdNetManager::Script_AssureProfileExists( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );
	assert( networkService->GetActiveUser()->GetState() == sdNetUser::US_ONLINE );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::AssureProfileExists : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	if ( !activeTask.Set( activeUser->GetProfile().AssureExists() ) ) {
		gameLocal.Printf( "SDNet::AssureProfileExists : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
================
sdNetManager::Script_StoreProfile
================
*/
void sdNetManager::Script_StoreProfile( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );
	assert( networkService->GetActiveUser()->GetState() == sdNetUser::US_ONLINE );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::StoreProfile : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	if ( !activeTask.Set( activeUser->GetProfile().Store() ) ) {
		gameLocal.Printf( "SDNet::StoreProfile : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
================
sdNetManager::Script_RestoreProfile
================
*/
void sdNetManager::Script_RestoreProfile( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );
	assert( networkService->GetActiveUser()->GetState() == sdNetUser::US_ONLINE );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::RestoreProfile : task pending\n" );
		return;
	}

	sdNetUser* activeUser = networkService->GetActiveUser();

	if ( !activeTask.Set( activeUser->GetProfile().Restore() ) ) {
		gameLocal.Printf( "SDNet::RestoreProfile : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}
#endif /* !SD_DEMO_BUILD */

/*
================
sdNetManager::Script_ValidateProfile
================
*/
void sdNetManager::Script_ValidateProfile( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idDict& properties = networkService->GetActiveUser()->GetProfile().GetProperties();

	stack.Push( true );
}

/*
================
sdNetManager::Script_FindServers
================
*/
void sdNetManager::Script_FindServers( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	findServerSource_e source;
	if( !sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, source ) ) {
		gameLocal.Error( "FindServers: source '%i' out of range", iSource );
		stack.Push( false );
		return;
	}

	if( source == FS_INTERNET && networkService->GetState() != sdNetService::SS_ONLINE ) {
		gameLocal.Warning( "FindServers: cannot search for internet servers while offline\n", iSource );
		stack.Push( false );
		return;
	}

	sdNetTask* task = NULL;
	idList< sdNetSession* >* netSessions = NULL;

	GetSessionsForServerSource( source, netSessions, task );

	if( netSessions == NULL ) {
		stack.Push( false );
		return;
	}

	if ( task != NULL ) {
		gameLocal.Printf( "SDNet::FindServers : Already scanning for servers\n" );
		stack.Push( false );
		return;
	}

	for ( int i = 0; i < netSessions->Num(); i ++ ) {
		networkService->GetSessionManager().FreeSession( (*netSessions)[ i ] );
	}	

	netSessions->SetGranularity( 1024 );
	netSessions->SetNum( 0, false );
	lastServerUpdateIndex = 0;

#if !defined( SD_DEMO_BUILD )
	CacheServersWithFriends();
#endif /* !SD_DEMO_BUILD */

	sdNetSessionManager::sessionSource_e managerSource;
	switch ( source ) {
		case FS_LAN:
			managerSource = sdNetSessionManager::SS_LAN;
			break;
		case FS_INTERNET: {
#if !defined( SD_DEMO_BUILD )
			managerSource = ShowRanked() ? sdNetSessionManager::SS_INTERNET_RANKED : sdNetSessionManager::SS_INTERNET_ALL;
#else
			managerSource = sdNetSessionManager::SS_INTERNET_ALL;
#endif /* !SD_DEMO_BUILD */
		}
	}
	if ( source != FS_HISTORY && source != FS_FAVORITES ) {
		task = networkService->GetSessionManager().FindSessions( *netSessions, managerSource );
		if ( task == NULL ) {
			gameLocal.Printf( "SDNet::FindServers : failed (%d)\n", networkService->GetLastError() );
			properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
			stack.Push( false );
			return;
		}
	}

	switch( source ) {
		case FS_INTERNET:
			findServersTask = task;
			break;
		case FS_LAN:
			findLANServersTask = task;
			break;
		case FS_HISTORY: {
				assert( networkService->GetActiveUser() != NULL );
				const idDict& dict = networkService->GetActiveUser()->GetProfile().GetProperties();
				sessionsHistory.SetGranularity( sdNetUser::MAX_SERVER_HISTORY );

				for( int i = 0; i < sdNetUser::MAX_SERVER_HISTORY; i++ ) {
					const idKeyValue* kv = dict.FindKey( va( "serverHistory%i", i ) );
					if( kv == NULL || kv->GetValue().IsEmpty() ) {
						continue;
					}
					netadr_t addr;
					if( sys->StringToNetAdr( kv->GetValue(), &addr, false ) ) {
						sdNetSession* session = networkService->GetSessionManager().AllocSession( &addr );
						sessionsHistory.Append( session );
					}
				}
				findHistoryServersTask = task = networkService->GetSessionManager().RefreshSessions( sessionsHistory );
				if ( task == NULL ) {
					gameLocal.Printf( "SDNet::FindServers : failed (%d)\n", networkService->GetLastError() );
					properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
					stack.Push( false );
					return;
				}
			}
			break;
		case FS_FAVORITES: {
			assert( networkService->GetActiveUser() != NULL );
			const idDict& dict = networkService->GetActiveUser()->GetProfile().GetProperties();

			int prefixLength = idStr::Length( "favorite_" );
			const idKeyValue* kv = dict.MatchPrefix( "favorite_" );

			while( kv != NULL ) {
				const char* value = kv->GetKey().c_str();
				value += prefixLength;
				if( *value != '\0' ) {
					netadr_t addr;
					if( sys->StringToNetAdr( value, &addr, false ) ) {
						sdNetSession* session = networkService->GetSessionManager().AllocSession( &addr );
						sessionsFavorites.Append( session );
					}
				}

				kv = dict.MatchPrefix( "favorite_", kv );
			}
			findFavoriteServersTask = task = networkService->GetSessionManager().RefreshSessions( sessionsFavorites );
			if ( task == NULL ) {
				gameLocal.Printf( "SDNet::FindServers : failed (%d)\n", networkService->GetLastError() );
				properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
				stack.Push( false );
				return;
			}
		}
		break;
	}

	stack.Push( true );
}


/*
============
sdNetManager::Script_RefreshServer
============
*/
void sdNetManager::Script_RefreshServer( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	int sessionIndex;
	stack.Pop( sessionIndex );

	idStr sessionStr;
	stack.Pop( sessionStr );

	netadr_t addr;

	if( sessionStr.IsEmpty() ) {
		sdNetSession* netSession = GetSession( iSource, sessionIndex );

		if ( refreshServerTask != NULL ) {
			refreshServerTask->Cancel();
			networkService->FreeTask( refreshServerTask );
		}
		if( netSession == NULL ) {
			stack.Push( 0.0f );
			return;
		}

		sys->StringToNetAdr( netSession->GetHostAddressString(), &addr, false );
	} else {
		sys->StringToNetAdr( sessionStr, &addr, false );
	}

	if( serverRefreshSession != NULL ) {
		networkService->GetSessionManager().FreeSession( serverRefreshSession );
	}

	serverRefreshSession = networkService->GetSessionManager().AllocSession( &addr );
	sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, serverRefreshSource );

	properties.SetServerRefreshComplete( false );
	refreshServerTask = networkService->GetSessionManager().RefreshSession( *serverRefreshSession );
	if ( refreshServerTask == NULL ) {
		gameLocal.Printf( "SDNet::RefreshServer : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
		stack.Push( 0.0f );
		return;
	}
	stack.Push( 1.0f );

}

/*
================
sdNetManager::Script_JoinServer
================
*/
void sdNetManager::Script_JoinServer( sdUIFunctionStack& stack ) {
	float fSource;
	stack.Pop( fSource );

	float sessionIndexFloat;
	stack.Pop( sessionIndexFloat );

	int sessionIndex = idMath::FtoiFast( sessionIndexFloat );

	sdNetSession* netSession = GetSession( fSource, sessionIndex );
	if( netSession != NULL ) {
		netSession->Join();
	}
}

#if !defined( SD_DEMO_BUILD )
/*
================
sdNetManager::Script_CheckKey
================
*/
void sdNetManager::Script_CheckKey( sdUIFunctionStack& stack ) {
	idStr keyCode;
	idStr checksum;
	stack.Pop( keyCode );
	stack.Pop( checksum );

	stack.Push( networkService->CheckKey( va( "%s %s", keyCode.c_str(), checksum.c_str() ) ) ? 1.0f : 0.0f );
}

/*
============
sdNetManager::Script_Friend_Init
============
*/
void sdNetManager::Script_Friend_Init( sdUIFunctionStack& stack ) {
	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendInit : task pending\n" );
		return;
	}

	initFriendsTask = networkService->GetFriendsManager().Init();
	if ( initFriendsTask == NULL ) {
		gameLocal.Printf( "SDNet::FriendInit : failed (%d)\n", networkService->GetLastError() );
	}
}

/*
============
sdNetManager::Script_Friend_ProposeFriendShip
============
*/
void sdNetManager::Script_Friend_ProposeFriendShip( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	idWStr reason;
	stack.Pop( reason );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendProposeFriendShip : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetFriendsManager().ProposeFriendship( userName, reason.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::FriendProposeFriendShip : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Friend_AcceptProposal
============
*/
void sdNetManager::Script_Friend_AcceptProposal( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendAcceptProposal : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetFriendsManager().AcceptProposal( userName.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::FriendAcceptProposal : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Friend_RejectProposal
============
*/
void sdNetManager::Script_Friend_RejectProposal( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendRejectProposal : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetFriendsManager().RejectProposal( userName ) ) ) {
		gameLocal.Printf( "SDNet::FriendRejectProposal : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Friend_RemoveFriend
============
*/
void sdNetManager::Script_Friend_RemoveFriend( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendRemoveFriend : task pending\n" );
		return;
	}
	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& invited = networkService->GetFriendsManager().GetInvitedFriendsList();

	// if the friend is pending, withdraw the proposal
	if( networkService->GetFriendsManager().FindFriend( invited, userName.c_str() ) != NULL ) {
		if ( !activeTask.Set( networkService->GetFriendsManager().WithdrawProposal( userName ) ) ) {
			gameLocal.Printf( "SDNet::FriendRemoveFriend : failed (%d)\n", networkService->GetLastError() );
			properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

			return;
		}
	} else {
		if ( !activeTask.Set( networkService->GetFriendsManager().RemoveFriend( userName ) ) ) {
			gameLocal.Printf( "SDNet::FriendRemoveFriend : failed (%d)\n", networkService->GetLastError() );
			properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

			return;
		}

	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Friend_SetBlockedStatus
============
*/
void sdNetManager::Script_Friend_SetBlockedStatus( sdUIFunctionStack& stack ) {
	idStr userName;
	int status;
	stack.Pop( userName );
	stack.Pop( status );

	assert( status >= sdNetFriend::BS_NO_BLOCK && status < sdNetFriend::BS_END );

	sdNetFriend::blockState_e blockStatus = static_cast< sdNetFriend::blockState_e >( status );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendSetBlockedStatus : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetFriendsManager().SetBlockedStatus( userName, blockStatus ) ) ) {
		gameLocal.Printf( "SDNet::FriendSetBlockedStatus : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Friend_SendIM
============
*/
void sdNetManager::Script_Friend_SendIM( sdUIFunctionStack& stack ) {
	idStr userName;
	idWStr message;

	stack.Pop( userName );
	stack.Pop( message );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendSendIM : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetFriendsManager().SendMessage( userName, message.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::FriendSendIM : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Friend_GetIMText
============
*/
void sdNetManager::Script_Friend_GetIMText( sdUIFunctionStack& stack ) {
	idStr userName;

	stack.Pop( userName );

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();

	if ( sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, userName ) ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_IM );
		if( ims.Num() ) {
			stack.Push( ims[ 0 ]->GetText() );
			return;
		}
	}
	stack.Push( L"" );
}

/*
============
sdNetManager::Script_Friend_GetProposalText
============
*/
void sdNetManager::Script_Friend_GetProposalText( sdUIFunctionStack& stack ) {
	idStr userName;

	stack.Pop( userName );

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetPendingFriendsList();

	if ( sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, userName ) ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_FRIENDSHIP_PROPOSAL );
		if( ims.Num() ) {
			stack.Push( ims[ 0 ]->GetText() );
			return;
		}
	}
	stack.Push( L"" );
}

/*
============
sdNetManager::Script_Friend_InviteFriend
============
*/
void sdNetManager::Script_Friend_InviteFriend( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::FriendInviteFriend : task pending\n" );
		return;
	}

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( networkService->GetFriendsManager().GetFriendsList(), userName.c_str() );

	if ( !activeTask.Set( networkService->GetFriendsManager().Invite( userName.c_str(), networkService->GetSessionManager().GetCurrentSessionAddress() ), &sdNetManager::OnFriendInvited, mate ) ) {
		gameLocal.Printf( "SDNet::FriendInviteFriend : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::OnFriendInvited
============
*/
void sdNetManager::OnFriendInvited( sdNetTask* task, void* parm ) {
	if ( task->GetErrorCode() == SDNET_NO_ERROR ) {
		sdNetFriend* mate = reinterpret_cast< sdNetFriend* >( parm );

		sdNetClientId netClientId;

		mate->GetNetClientId( netClientId );

		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_RESERVECLIENTSLOT );
		msg.WriteLong( netClientId.id[0] );
		msg.WriteLong( netClientId.id[1] );
		msg.Send();
	}
}

/*
============
sdNetManager::Script_Friend_ChooseContextAction
============
*/
void sdNetManager::Script_Friend_ChooseContextAction( sdUIFunctionStack& stack ) {
	assert( activeMessage == NULL );

	idStr friendName;
	stack.Pop( friendName );
	if ( friendName.IsEmpty() ) {
		stack.Push( sdNetProperties::FCA_NONE );
		return;
	}

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& pending = networkService->GetFriendsManager().GetPendingFriendsList();
	sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( pending, friendName );
	if ( mate != NULL ) {
		sdNetMessage* message = mate->GetMessageQueue().GetMessages();
		if ( message != NULL ) {
			switch( message->GetType() ) {
				case sdNetMessage::MT_FRIENDSHIP_PROPOSAL:
					activeMessage = message;
					stack.Push( sdNetProperties::FCA_RESPOND_TO_PROPOSAL );
					return;
				default:
					assert( 0 );
			}
		}

		// nothing to do
		stack.Push( sdNetProperties::FCA_NONE );
		return;
	}

	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
	mate = networkService->GetFriendsManager().FindFriend( friends, friendName );
	if ( mate != NULL ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_IM );
		if( ims.Num() ) {
			activeMessage = ims[ 0 ];
			stack.Push( sdNetProperties::FCA_READ_IM );
			return;
		} else {
			sdNetMessage* message = mate->GetMessageQueue().GetMessages();
			if ( message != NULL ) {
				switch( message->GetType() ) {
				case sdNetMessage::MT_BLOCKED:
					activeMessage = message;
					stack.Push( sdNetProperties::FCA_BLOCKED );
					return;
				case sdNetMessage::MT_UNBLOCKED:
					activeMessage = message;
					stack.Push( sdNetProperties::FCA_UNBLOCKED );
					return;
				case sdNetMessage::MT_SESSION_INVITE:
					activeMessage = message;
					stack.Push( sdNetProperties::FCA_RESPOND_TO_INVITE );
					return;
				}
			}
		}

		// nothing to do, send on a message
		stack.Push( sdNetProperties::FCA_SEND_IM );
		return;
	}

	const sdNetFriendsList& blocked = networkService->GetFriendsManager().GetBlockedList();
	mate = networkService->GetFriendsManager().FindFriend( blocked, friendName );
	if ( mate != NULL ) {
		int x = 0;
	}

	stack.Push( sdNetProperties::FCA_NONE );
}

/*
============
sdNetManager::Script_Friend_NumAvailableIMs
============
*/
void sdNetManager::Script_Friend_NumAvailableIMs( sdUIFunctionStack& stack ) {

	idStr friendName;
	stack.Pop( friendName );
	if ( friendName.IsEmpty() ) {
		stack.Push( 0 );
		return;
	}

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
	sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName );
	if ( mate != NULL ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_IM );
		stack.Push( ims.Num() );
		return;
	}

	stack.Push( 0 );
}

/*
============
sdNetManager::Script_DeleteActiveMessage
============
*/
void sdNetManager::Script_DeleteActiveMessage( sdUIFunctionStack& stack ) {
	assert( activeMessage != NULL );
	assert( activeMessage->GetSenderQueue() != NULL );

	if ( sdNetMessageQueue* senderQueue = activeMessage->GetSenderQueue() ) {
		senderQueue->RemoveMessage( activeMessage );
	}
	activeMessage = NULL;
}

/*
============
sdNetManager::Script_IsFriend
============
*/
void sdNetManager::Script_IsFriend( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	float result = 0.0f;

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
	const sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName.c_str() );

	stack.Push( mate == NULL ? 0.0f : 1.0f );
}

/*
============
sdNetManager::Script_IsPendingFriend
============
*/
void sdNetManager::Script_IsPendingFriend( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	float result = 0.0f;

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetPendingFriendsList();
	const sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName.c_str() );

	stack.Push( mate != NULL );
}

/*
============
sdNetManager::Script_IsInvitedFriend
============
*/
void sdNetManager::Script_IsInvitedFriend( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	float result = 0.0f;

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetInvitedFriendsList();
	const sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName.c_str() );

	stack.Push( mate != NULL );
}

/*
============
sdNetManager::Script_Friend_GetBlockedStatus
============
*/
void sdNetManager::Script_Friend_GetBlockedStatus( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	const sdNetFriendsList& blockedFriends = networkService->GetFriendsManager().GetBlockedList();
	const sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( blockedFriends, friendName.c_str() );
	if( mate == NULL ) {
		stack.Push( sdNetFriend::BS_NO_BLOCK );
		return;
	}

	stack.Push( mate->GetBlockedState() );
}
#endif /* !SD_DEMO_BUILD */

/*
============
sdNetManager::Script_QueryServerInfo
============
*/
void sdNetManager::Script_QueryServerInfo( sdUIFunctionStack& stack ) {
	float fSource;
	stack.Pop( fSource );

	float sessionIndexFloat;
	stack.Pop( sessionIndexFloat );

	idStr key;
	stack.Pop( key );

	idStr defaultValue;
	stack.Pop( defaultValue );

	if( fSource == FS_CURRENT ) {
		if( key.Icmp( "_address" ) == 0 ) {
			netadr_t addr = networkSystem->ClientGetServerAddress();
			if( addr.type == NA_BAD ) {
				stack.Push( defaultValue );
				return;
			}
			stack.Push( sys->NetAdrToString( addr ) );
			return;
		}

		stack.Push( gameLocal.serverInfo.GetString( key.c_str(), defaultValue.c_str() ) );
		return;
	}

	sdNetSession* netSession = GetSession( fSource, idMath::FtoiFast( sessionIndexFloat ) );

	if( netSession == NULL ) {
		stack.Push( defaultValue.c_str() );
		return;
	}

	if( key.Icmp( "_time" ) == 0 ) {
		stack.Push( idStr::MS2HMS( netSession->GetSessionTime() ) );
		return;
	}

	if( key.Icmp( "_ranked" ) == 0 ) {
		stack.Push( netSession->IsRanked() ? "1" : "0" );
		return;
	}

	stack.Push( netSession->GetServerInfo().GetString( key.c_str(), defaultValue.c_str() ) );
}

/*
============
sdNetManager::Script_QueryMapInfo
============
*/
void sdNetManager::Script_QueryMapInfo( sdUIFunctionStack& stack ) {
	float fSource;
	stack.Pop( fSource );

	float sessionIndexFloat;
	stack.Pop( sessionIndexFloat );

	idStr key;
	stack.Pop( key );

	idStr defaultValue;
	stack.Pop( defaultValue );

	const sdNetSession* netSession = GetSession( fSource, idMath::FtoiFast( sessionIndexFloat ) );

	if( netSession == NULL ) {
		stack.Push( defaultValue.c_str() );
		return;
	}

	const idDict* mapInfo = GetMapInfo( *netSession );

	if( mapInfo == NULL ) {
		stack.Push( defaultValue.c_str() );
		return;
	}

	stack.Push( mapInfo->GetString( key.c_str(), defaultValue.c_str() ) );
}


/*
============
sdNetManager::Script_QueryGameType
============
*/
void sdNetManager::Script_QueryGameType( sdUIFunctionStack& stack ) {
	float fSource;
	stack.Pop( fSource );

	float sessionIndexFloat;
	stack.Pop( sessionIndexFloat );

	sdNetSession* netSession = GetSession( fSource, idMath::FtoiFast( sessionIndexFloat ) );
	if( netSession == NULL ) {
		stack.Push( L"" );
		return;
	}

	const char* siRules = netSession->GetServerInfo().GetString( "si_rules" );
	idWStr gameType;
	GetGameType( siRules, gameType );

	stack.Push( gameType.c_str() );
}


/*
============
sdNetManager::GetMapInfo
============
*/
const idDict* sdNetManager::GetMapInfo( const sdNetSession& netSession ) {
	idStr mapName = netSession.GetServerInfo().GetString( "si_map" );
	mapName.StripFileExtension();

	return gameLocal.mapMetaDataList->FindMetaData( mapName );
}

/*
============
sdNetManager::GetSession
============
*/
sdNetSession* sdNetManager::GetSession( float fSource, int index ) {
	int iSource = idMath::Ftoi( fSource );
	if( iSource < sdNetManager::FS_LAN || iSource >= sdNetManager::FS_MAX ) {
		gameLocal.Error( "GetSession: source '%i' out of range", iSource );
		return NULL;
	}

	if( iSource == FS_CURRENT ) {
		return gameSession;
	}

	idList< sdNetSession* >* netSessions = NULL;
	sdNetTask* task;

	GetSessionsForServerSource( static_cast< findServerSource_e >( iSource ), netSessions, task );

	if( netSessions == NULL || index < 0 || index >= netSessions->Num() ) {
		return NULL;
	}

	if( task != NULL ) {
		task->AcquireLock();
	}

	sdNetSession* netSession = (*netSessions)[ index ];

	if( task != NULL ) {
		task->ReleaseLock();
	}

	return netSession;
}

/*
============
sdNetManager::Script_RequestStats
============
*/
void sdNetManager::Script_RequestStats( sdUIFunctionStack& stack ) {
	bool globalOnly;
	stack.Pop( globalOnly );
	stack.Push( sdGlobalStatsTracker::GetInstance().StartStatsRequest( globalOnly ) );
}

/*
============
sdNetManager::Script_GetStat
============
*/
void sdNetManager::Script_GetStat( sdUIFunctionStack& stack ) {
	idStr key;
	stack.Pop( key );

	const sdNetStatKeyValList& stats = sdGlobalStatsTracker::GetInstance().GetLocalStats();
	for( int i = 0; i < stats.Num(); i++ ) {
		const sdNetStatKeyValue& kv = stats[ i ];
		if( kv.key->Icmp( key.c_str() ) == 0 ) {
			switch( kv.type ) {
				case sdNetStatKeyValue::SVT_FLOAT:
					stack.Push( kv.val.f );
					return;
				case sdNetStatKeyValue::SVT_INT:
					stack.Push( kv.val.i );
					return;
			}
		}
	}
	stack.Push( 0.0f );
}

/*
============
sdNetManager::Script_GetPlayerAward
============
*/
void sdNetManager::Script_GetPlayerAward( sdUIFunctionStack& stack ) {
	int iReward;
	stack.Pop( iReward );

	int iStat;
	stack.Pop( iStat );

	playerReward_e reward;
	if( !sdIntToContinuousEnum< playerReward_e >( iReward, PR_MIN, PR_MAX, reward ) || iReward >= gameLocal.endGameStats.Num() ) {
		stack.Push( L"" );
		return;
	}

	if( gameLocal.endGameStats[ iReward ].name.IsEmpty() ) {
		stack.Push( L"" );
		return;
	}


	playerStat_e stat;
	if( sdIntToContinuousEnum< playerStat_e >( iStat, PS_MIN, PS_MAX, stat ) ) {
		switch( stat ) {
			case PS_NAME:
				stack.Push( va( L"%hs", gameLocal.endGameStats[ iReward ].name.c_str() ) );
				break;
			case PS_XP:
				if ( iReward == PR_LEAST_XP ) {
					stack.Push( L"" );
				} else {
					float value = gameLocal.endGameStats[ iReward ].value;
					if ( value >= 1000.f ) {
						idWStrList parms;
						parms.Alloc() = va( L"%.1f", ( value / 1000.f ) );
						idWStr result = common->LocalizeText( "guis/mainmenu/kilo", parms ).c_str();
						if( iReward == PR_ACCURACY_HIGH || iReward == PR_ACCURACY_LOW ) {
							result += L"%";
						}
						stack.Push( result.c_str() );
					} else {
						if( iReward == PR_ACCURACY_HIGH || iReward == PR_ACCURACY_LOW ) {
							stack.Push( va( L"%i%%", idMath::Ftoi( value ) ) );
						} else {
							stack.Push( va( L"%i", idMath::Ftoi( value ) ) );
						}
					}
				}
				break;
			case PS_RANK: {
				const sdDeclRank* rank = gameLocal.endGameStats[ iReward ].rank;
				if( rank != NULL ) {
					stack.Push( va( L"%hs", rank->GetMaterial() ) );
				} else {
					stack.Push( L"_default" );
				}
			}
			break;
			case PS_TEAM: {
				const sdTeamInfo* team = gameLocal.endGameStats[ iReward ].team;
				if( team != NULL ) {
					stack.Push( va( L"%hs", team->GetLookupName() ) );
				} else {
					stack.Push( L"" );
				}
			}
			  break;
		}
	} else {
		stack.Push( L"" );
	}
}

/*
============
sdNetManager::Script_QueryXPStats
============
*/
void sdNetManager::Script_QueryXPStats( sdUIFunctionStack& stack ) {
	idStr profKey;
	stack.Pop( profKey );

	bool total;
	stack.Pop( total );

	if( profKey.IsEmpty() ) {
		gameLocal.Warning( "sdNetManager::Script_QueryXPStats: empty proficiency key" );
		stack.Push( "" );
		return;
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( localPlayer != NULL ) {
		const sdProficiencyTable& table = localPlayer->GetProficiencyTable();
		const sdDeclProficiencyType* prof = gameLocal.declProficiencyTypeType.LocalFind( profKey.c_str() );
		int index = prof->Index();
		float xp;
		if( total ) {
			xp = table.GetPoints( index );
		} else {
			xp = table.GetPointsSinceBase( index );
		}
		stack.Push( va( "%i", idMath::Ftoi( xp ) ) );
		return;
	}
	stack.Push( "" );
}

/*
============
sdNetManager::Script_QueryXPTotals
============
*/
void sdNetManager::Script_QueryXPTotals( sdUIFunctionStack& stack ) {
	bool total;
	stack.Pop( total );
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( localPlayer != NULL ) {
		const sdProficiencyTable& table = localPlayer->GetProficiencyTable();

		float sum = 0;
		const sdGameRules::mapData_t* data = gameLocal.rules->GetCurrentMapData();
		for( int i = 0; i < gameLocal.declProficiencyTypeType.Num(); i++ ) {
			const sdDeclProficiencyType* prof = gameLocal.declProficiencyTypeType.LocalFindByIndex( i );

			if( total ) {
				sum += table.GetPoints( prof->Index() );
			} else {
				sum += table.GetPointsSinceBase( prof->Index() );
			}
		}
		stack.Push( va( "%i", idMath::Ftoi( sum ) ) );
		return;
	}

	stack.Push( "" );
}

#if !defined( SD_DEMO_BUILD )
/*
============
sdNetManager::Script_Team_Init
============
*/
void sdNetManager::Script_Team_Init( sdUIFunctionStack& stack ) {
	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamInit : task pending\n" );
		return;
	}

	initTeamsTask = networkService->GetTeamManager().Init();
	if ( initTeamsTask == NULL ) {
		gameLocal.Printf( "SDNet::TeamInit : failed (%d)\n", networkService->GetLastError() );
	}
}


/*
============
sdNetManager::Script_Team_CreateTeam
============
*/
void sdNetManager::Script_Team_CreateTeam( sdUIFunctionStack& stack ) {
	idStr teamName;
	stack.Pop( teamName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::CreateTeam : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().CreateTeam( teamName.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::CreateTeam : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_ProposeMembership
============
*/
void sdNetManager::Script_Team_ProposeMembership( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	idWStr reason;
	stack.Pop( reason );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamProposeMembership : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().ProposeMembership( userName.c_str(), reason.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamProposeMembership : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_AcceptMembership
============
*/
void sdNetManager::Script_Team_AcceptMembership( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	assert( activeMessage == NULL );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamAcceptMembership : task pending\n" );
		stack.Push( false );
		return;
	}

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	const sdNetTeamMemberList& list = networkService->GetTeamManager().GetPendingInvitesList();
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( list, userName.c_str() );
	if( mate == NULL ) {
		gameLocal.Printf( "SDNet::TeamAcceptMembership : couldn't find '%s'\n", userName.c_str() );
		stack.Push( false );
		return;
	}

	sdNetMessage* message = mate->GetMessageQueue().GetMessages();
	if( message == NULL || message->GetType() != sdNetMessage::MT_MEMBERSHIP_PROPOSAL ) {
		gameLocal.Printf( "SDNet::TeamAcceptMembership : message wasn't a membership proposal\n" );
		stack.Push( false );
		return;
	}
	assert( message->GetDataSize() == sizeof( sdNetTeamInvite ) );

	const sdNetTeamInvite& teamInvite = reinterpret_cast< const sdNetTeamInvite& >( *message->GetData() );

	if ( !activeTask.Set( networkService->GetTeamManager().AcceptMembership( userName, teamInvite.netTeamId ) ) ) {
		gameLocal.Printf( "SDNet::TeamAcceptMembership : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
		stack.Push( false );
		return;
	}
	activeMessage = message;

	properties.SetTaskActive( true );
	stack.Push( true );
}

/*
============
sdNetManager::Script_Team_RejectMembership
============
*/
void sdNetManager::Script_Team_RejectMembership( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	assert( activeMessage == NULL );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamRejectMembership : task pending\n" );
		stack.Push( 0 );
		return;
	}

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( networkService->GetTeamManager().GetPendingInvitesList(), userName.c_str() );
	if( mate == NULL ) {
		gameLocal.Printf( "SDNet::TeamRejectMembership : couldn't find '%s'\n", userName.c_str() );
		stack.Push( 0 );
		return;
	}

	sdNetMessage* message = mate->GetMessageQueue().GetMessages();
	if( message == NULL || message->GetType() != sdNetMessage::MT_MEMBERSHIP_PROPOSAL ) {
		gameLocal.Printf( "SDNet::TeamRejectMembership : message wasn't a membership proposal\n" );
		stack.Push( 0 );
		return;
	}

	const sdNetTeamInvite& teamInvite = reinterpret_cast< const sdNetTeamInvite& >( *message->GetData() );

	if ( !activeTask.Set( networkService->GetTeamManager().RejectMembership( userName, teamInvite.netTeamId ) ) ) {
		gameLocal.Printf( "SDNet::TeamRejectMembership : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
		stack.Push( 0 );
		return;
	}

	activeMessage = message;

	properties.SetTaskActive( true );
	stack.Push( 1 );
}

/*
============
sdNetManager::Script_Team_RemoveMember
============
*/
void sdNetManager::Script_Team_RemoveMember( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamRemoveMember : task pending\n" );
		return;
	}

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	const sdNetTeamMemberList& pending = networkService->GetTeamManager().GetPendingMemberList();
	if( networkService->GetTeamManager().FindMember( pending, userName.c_str() ) != NULL ) {
		if ( !activeTask.Set( networkService->GetTeamManager().WithdrawMembership( userName.c_str() ) ) ) {
			gameLocal.Printf( "SDNet::TeamRemoveMember : failed (%d)\n", networkService->GetLastError() );
			properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

			return;
		}
	} else {
		if ( !activeTask.Set( networkService->GetTeamManager().RemoveMember( userName.c_str() ) ) ) {
			gameLocal.Printf( "SDNet::TeamRemoveMember : failed (%d)\n", networkService->GetLastError() );
			properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

			return;
		}
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_SendMessage
============
*/
void sdNetManager::Script_Team_SendMessage( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	idWStr text;
	stack.Pop( text );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamSendMessage : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().SendMessage( userName.c_str(), text.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamSendMessage : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_BroadcastMessage
============
*/
void sdNetManager::Script_Team_BroadcastMessage( sdUIFunctionStack& stack ) {
	idWStr text;
	stack.Pop( text );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamBroadcastMessage : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().BroadcastMessage( text.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamBroadcastMessage : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_Invite
============
*/
void sdNetManager::Script_Team_Invite( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamInvite : task pending\n" );
		return;
	}

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( networkService->GetTeamManager().GetMemberList(), userName.c_str() );

	if ( !activeTask.Set( networkService->GetTeamManager().Invite( userName.c_str(), networkService->GetSessionManager().GetCurrentSessionAddress() ), &sdNetManager::OnMemberInvited, mate ) ) {
		gameLocal.Printf( "SDNet::TeamInvite : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::OnMemberInvited
============
*/
void sdNetManager::OnMemberInvited( sdNetTask* task, void* parm ) {
	if ( task->GetErrorCode() == SDNET_NO_ERROR ) {
		sdNetTeamMember* mate = reinterpret_cast< sdNetTeamMember* >( parm );

		sdNetClientId netClientId;

		mate->GetNetClientId( netClientId );

		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_RESERVECLIENTSLOT );
		msg.WriteLong( netClientId.id[0] );
		msg.WriteLong( netClientId.id[1] );
		msg.Send();
	}
}

/*
============
sdNetManager::Script_Team_PromoteMember
============
*/
void sdNetManager::Script_Team_PromoteMember( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamPromoteMember : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().PromoteMember( userName.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamPromoteMember : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_DemoteMember
============
*/
void sdNetManager::Script_Team_DemoteMember( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamDemoteMember : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().DemoteMember( userName.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamDemoteMember : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_TransferOwnership
============
*/
void sdNetManager::Script_Team_TransferOwnership( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamTransferOwnership : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().TransferOwnership( userName.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamTransferOwnership : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_DisbandTeam
============
*/
void sdNetManager::Script_Team_DisbandTeam( sdUIFunctionStack& stack ) {
	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamDisbandTeam : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().DisbandTeam() ) ) {
		gameLocal.Printf( "SDNet::TeamDisbandTeam : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}

/*
============
sdNetManager::Script_Team_LeaveTeam
============
*/
void sdNetManager::Script_Team_LeaveTeam( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	if ( activeTask.task != NULL ) {
		gameLocal.Printf( "SDNet::TeamLeaveTeam : task pending\n" );
		return;
	}

	if ( !activeTask.Set( networkService->GetTeamManager().LeaveTeam( userName.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::TeamLeaveTeam : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );

		return;
	}

	properties.SetTaskActive( true );
}
#endif /* !SD_DEMO_BUILD */

/*
============
sdNetManager::Script_ApplyNumericFilter
============
*/
void sdNetManager::Script_ApplyNumericFilter( sdUIFunctionStack& stack ) {
	float type;
	stack.Pop( type );

	float state;
	stack.Pop( state );

	float op;
	stack.Pop( op );

	float bin;
	stack.Pop( bin );

	bool success = true;
	serverNumericFilter_t& filter = *numericFilters.Alloc();

	success &= sdFloatToContinuousEnum< serverFilter_e >( type, SF_MIN, SF_MAX, filter.type );
	success &= sdFloatToContinuousEnum< serverFilterState_e >( state, SFS_MIN, SFS_MAX, filter.state );
	success &= sdFloatToContinuousEnum< serverFilterOp_e >( op, SFO_MIN, SFO_MAX, filter.op );
	success &= sdFloatToContinuousEnum< serverFilterResult_e >( bin, SFR_MIN, SFR_MAX, filter.resultBin );
	stack.Pop( filter.value );

	if( !success ) {
		gameLocal.Warning( "ApplyNumericFilter: invalid enum, filter not applied" );
		numericFilters.RemoveIndexFast( numericFilters.Num() - 1 );
	}
}

/*
============
sdNetManager::Script_ApplyStringFilter
============
*/
void sdNetManager::Script_ApplyStringFilter( sdUIFunctionStack& stack ) {
	idStr cvar;
	stack.Pop( cvar );

	float state;
	stack.Pop( state );

	float op;
	stack.Pop( op );

	float bin;
	stack.Pop( bin );

	bool success = true;
	serverStringFilter_t& filter = *stringFilters.Alloc();
	success &= sdFloatToContinuousEnum< serverFilterState_e >( state, SFS_MIN, SFS_MAX, filter.state );
	success &= sdFloatToContinuousEnum< serverFilterOp_e >( op, SFO_MIN, SFO_MAX, filter.op );
	success &= sdFloatToContinuousEnum< serverFilterResult_e >( bin, SFR_MIN, SFR_MAX, filter.resultBin );
	filter.cvar = cvar;
	stack.Pop( filter.value );

	if( !success ) {
		gameLocal.Warning( "ApplyStringFilter: invalid enum, filter not applied" );
		stringFilters.RemoveIndexFast( stringFilters.Num() - 1 );
	}
}

/*
============
sdNetManager::Script_ClearFilters
============
*/
void sdNetManager::Script_ClearFilters( sdUIFunctionStack& stack ) {
	numericFilters.Clear();
	stringFilters.Clear();
}


/*
============
sdNetManager::DoFiltering
============
*/
bool sdNetManager::DoFiltering( const sdNetSession& netSession ) const {
	for( int i = 0; i < unfilteredSessions.Num(); i++ ) {
		const netadr_t& addr = unfilteredSessions[ i ];
		const netadr_t& other = netSession.GetAddress();
		if( addr == other ) {
			return false;
		}
	}
	return true;
}

/*
============
sdNetManager::SessionIsFiltered
============
*/
bool sdNetManager::SessionIsFiltered( const sdNetSession& netSession ) const {
	if( numericFilters.Empty() && stringFilters.Empty() ) {
		return false;
	}

	assert( networkService->GetActiveUser() != NULL );
	sdNetUser* activeUser = networkService->GetActiveUser();

	bool visible = true;
	bool orFilters = false;
	bool orSet = false;

	for( int i = 0; i < numericFilters.Num() && visible; i++ ) {
		const serverNumericFilter_t& filter = numericFilters[ i ];
		if( filter.state == SFS_DONTCARE ) {
			continue;
		}

		float value = 0.0f;
		switch( filter.type ) {
			case SF_PASSWORDED:
				value = netSession.GetServerInfo().GetBool( "si_needPass", "0" ) ? 1.0f : 0.0f;
				break;
			case SF_PUNKBUSTER:
				value = netSession.GetServerInfo().GetBool( "net_serverPunkbusterEnabled", "0" ) ? 1.0f : 0.0f;
				break;
			case SF_FRIENDLYFIRE:
				value = netSession.GetServerInfo().GetBool( "si_teamDamage", "0" ) ? 1.0f : 0.0f;
				break;
			case SF_AUTOBALANCE:
				value = netSession.GetServerInfo().GetBool( "si_teamForceBalance", "0" ) ? 1.0f : 0.0f;
				break;
			case SF_PURE:
				value = netSession.GetServerInfo().GetBool( "si_pure", "0" ) ? 1.0f : 0.0f;
				break;
			case SF_LATEJOIN:
				value = netSession.GetServerInfo().GetBool( "si_allowLateJoin", "0" ) ? 1.0f : 0.0f;
				break;
			case SF_EMPTY:
				value = netSession.GetNumClients() == 0 ? 1.0f : 0.0f;
				break;
			case SF_FULL:
				value = netSession.GetNumClients() == netSession.GetServerInfo().GetInt( "si_maxPlayers" );
				break;
			case SF_PING:
				value = netSession.GetPing();
				break;
			case SF_BOTS:
				value = ( ( netSession.GetNumClients() == 0 ) && ( netSession.GetNumBotClients() > 0 ) ) ? 1.0f : 0.0f;
				break;
			case SF_FAVORITE:
				value = activeUser->GetProfile().GetProperties().GetBool( va( "favorite_%s", netSession.GetHostAddressString() ), "0" );
				break;
#if !defined( SD_DEMO_BUILD )
			case SF_RANKED:
				value = netSession.IsRanked() ? 1.0f : 0.0f;
				break;
			case SF_FRIENDS:
				value = AnyFriendsOnServer( netSession ) ? 1.0f : 0.0f;
				break;
#endif /* !SD_DEMO_BUILD */
			case SF_PLAYERCOUNT:
				value = netSession.GetNumClients() - netSession.GetNumBotClients();
				break;
			case SF_MODS:
				value = ( netSession.GetServerInfo().GetString( "fs_game", "" )[ 0 ] != '\0' ) ? 1.0f : 0.0f;
				break;
		}

		bool result = false;
		switch( filter.op ) {
			case SFO_EQUAL:
				result = idMath::Fabs( value - filter.value ) < idMath::FLT_EPSILON;
				break;
			case SFO_NOT_EQUAL:
				result = idMath::Fabs( value - filter.value ) >= idMath::FLT_EPSILON;
				break;
			case SFO_LESS:
				result = value < filter.value;
				break;
			case SFO_GREATER:
				result = value > filter.value;
				break;
		}

		if( filter.state == SFS_SHOWONLY && !result ) {
			visible = false;
			break;
		}

		if( filter.resultBin == SFR_OR ) {
			orFilters |= result;
			orSet = true;
		} else if( result && filter.state == SFS_HIDE ) {
			visible = false;
			break;
		}
	}

	for( int i = 0; i < stringFilters.Num() && visible; i++ ) {
		const serverStringFilter_t& filter = stringFilters[ i ];
		if( filter.state == SFS_DONTCARE ) {
			continue;
		}

		const char* value = netSession.GetServerInfo().GetString( filter.cvar.c_str() );

		// allow for filtering the pretty name
		if( gameLocal.mapMetaDataList != NULL && filter.cvar.Icmp( "si_map" ) == 0 ) {
			idStr prettyName( value );
			prettyName.StripFileExtension();
			if ( const idDict* mapInfo = gameLocal.mapMetaDataList->FindMetaData( prettyName ) ) {
				value = mapInfo->GetString( "pretty_name", value );
			}
		}
		builder.Clear();
		builder.AppendNoColors( value );

		bool result = false;

		switch( filter.op ) {
			case SFO_EQUAL:
				result = idStr::IcmpNoColor( builder.c_str(), filter.value.c_str() ) == 0;
				break;
			case SFO_NOT_EQUAL:
				result = idStr::IcmpNoColor( builder.c_str(), filter.value.c_str() ) != 0;
				break;
			case SFO_CONTAINS:
				result = idStr::FindText( builder.c_str(), filter.value.c_str(), false ) != idStr::INVALID_POSITION;
				break;
			case SFO_NOT_CONTAINS:
				result = idStr::FindText( builder.c_str(), filter.value.c_str(), false ) == idStr::INVALID_POSITION;
				break;
		}

		if( filter.state == SFS_SHOWONLY && !result ) {
 			visible = false;
			break;
		}
		if( result && filter.state != SFS_HIDE && filter.resultBin == SFR_OR ) {
			orFilters |= result;
			orSet = true;
		} else if( result && filter.state == SFS_HIDE ) {
			visible = false;
			break;
		}
	}

	if( orSet ) {
		visible &= orFilters;
	}

	return !visible;
}

/*
============
sdNetManager::Script_SaveFilters
============
*/
void sdNetManager::Script_SaveFilters( sdUIFunctionStack& stack ) {
	idStr prefix;
	stack.Pop( prefix );
	if( networkService->GetActiveUser() == NULL ) {
		assert( networkService->GetActiveUser() != NULL );
		gameLocal.Warning( "Script_SaveUser: no active user" );
		return;
	}

	idDict& dict = networkService->GetActiveUser()->GetProfile().GetProperties();

	idStr temp;
	for( int i = 0; i < numericFilters.Num(); i++ ) {
		const serverNumericFilter_t& filter = numericFilters[ i ];
		temp = va( "%i %i %i %i %f", filter.type, filter.op, filter.state, filter.resultBin, filter.value );
		dict.Set( va( "filter_%s_numeric_%i", prefix.c_str(), i ), temp.c_str() );
	}

	for( int i = 0; i < stringFilters.Num(); i++ ) {
		const serverStringFilter_t& filter = stringFilters[ i ];
		// limit user input to a sensible length
		sdStringBuilder_Heap buffer;
		buffer.Append( filter.value.c_str(), 64 );

		temp = va( "%s %i %i %i %s", filter.cvar.c_str(), filter.op, filter.state, filter.resultBin, buffer.c_str() );
		dict.Set( va( "filter_%s_string_%i", prefix.c_str(), i ), temp.c_str() );
	}
}

/*
============
sdNetManager::Script_LoadFilters
============
*/
void sdNetManager::Script_LoadFilters( sdUIFunctionStack& stack ) {
	idStr prefix;
	stack.Pop( prefix );

	if( networkService->GetActiveUser() == NULL ) {
		assert( networkService->GetActiveUser() != NULL );
		gameLocal.Warning( "Script_SaveUser: no active user" );
		return;
	}

	numericFilters.Clear();
	stringFilters.Clear();

	idDict& dict = networkService->GetActiveUser()->GetProfile().GetProperties();

	idToken token;

	int i = 0;

	const idKeyValue* kv = dict.MatchPrefix( va( "filter_%s_numeric_%i", prefix.c_str(), i ) );
	while( kv != NULL ) {
		serverNumericFilter_t& filter = *numericFilters.Alloc();

		idLexer src( kv->GetValue().c_str(), kv->GetValue().Length(), "Script_LoadFilters", LEXFL_NOERRORS | LEXFL_ALLOWMULTICHARLITERALS );

		bool success = true;
		if( src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilter_e >( token.GetIntValue(), SF_MIN, SF_MAX, filter.type );
		} else {
			success = false;
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilterOp_e >( token.GetIntValue(), SFO_MIN, SFO_MAX, filter.op );
		} else {
			success = false;
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilterState_e >( token.GetIntValue(), SFS_MIN, SFS_MAX, filter.state );
		} else {
			success = false;
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilterResult_e >( token.GetIntValue(), SFR_MIN, SFR_MAX, filter.resultBin );
		} else {
			success = false;
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_FLOAT, &token ) ) {
			filter.value = token.GetFloatValue();
		} else {
			success = false;
		}

		if( !success ) {
			gameLocal.Warning( "LoadFilters: Invalid filter stream for '%s'", kv->GetKey().c_str() );
			numericFilters.RemoveIndexFast( numericFilters.Num() - 1 );
		}

		i++;
		kv = dict.MatchPrefix( va( "filter_%s_numeric_%i", prefix.c_str(), i ) );
	}

	i = 0;
	kv = dict.MatchPrefix( va( "filter_%s_string_%i", prefix.c_str(), i ) );
	while( kv != NULL ) {
		serverStringFilter_t& filter = *stringFilters.Alloc();

		idLexer src( kv->GetValue().c_str(), kv->GetValue().Length(), "Script_LoadFilters", LEXFL_NOERRORS );

		bool success = true;
		if( src.ExpectTokenType( TT_NAME, 0, &token ) ) {
			filter.cvar = token;
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilterOp_e >( token.GetIntValue(), SFO_MIN, SFO_MAX, filter.op );
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilterState_e >( token.GetIntValue(), SFS_MIN, SFS_MAX, filter.state );
		}

		if( success && src.ExpectTokenType( TT_NUMBER, TT_INTEGER, &token ) ) {
			success &= sdFloatToContinuousEnum< serverFilterResult_e >( token.GetIntValue(), SFR_MIN, SFR_MAX, filter.resultBin );
		}

		if( success ) {
			src.ParseRestOfLine( filter.value );
		}

		if( !success ) {
			gameLocal.Warning( "LoadFilters: Invalid filter stream for '%s'", kv->GetKey().c_str() );
			numericFilters.RemoveIndexFast( stringFilters.Num() - 1 );
		}

		i++;
		kv = dict.MatchPrefix( va( "filter_%s_string_%i", prefix.c_str(), i ) );
	}

}

/*
============
sdNetManager::Script_QueryNumericFilterValue
============
*/
void sdNetManager::Script_QueryNumericFilterValue( sdUIFunctionStack& stack ) {
	float fType;
	stack.Pop( fType );

	serverFilter_e type = SF_MIN;
	if( sdFloatToContinuousEnum< serverFilter_e >( fType, SF_MIN, SF_MAX, type ) ) {
		for( int i = 0; i < numericFilters.Num(); i++ ) {
			const serverNumericFilter_t& filter = numericFilters[ i ];
			if( filter.type == type ) {
				stack.Push( filter.value );
				return;
			}
		}
	}
	stack.Push( 0.0f );
}

/*
============
sdNetManager::Script_QueryNumericFilterState
============
*/
void sdNetManager::Script_QueryNumericFilterState( sdUIFunctionStack& stack ) {
	float fType;
	stack.Pop( fType );

	float value;
	stack.Pop( value );

	serverFilter_e type = SF_MIN;
	if( sdFloatToContinuousEnum< serverFilter_e >( fType, SF_MIN, SF_MAX, type ) ) {
		for( int i = 0; i < numericFilters.Num(); i++ ) {
			const serverNumericFilter_t& filter = numericFilters[ i ];
			if( filter.type == type ) {
				stack.Push( filter.state );
				return;
			}
		}
	}

	stack.Push( SFS_DONTCARE );
}

/*
============
sdNetManager::Script_QueryStringFilterState
============
*/
void sdNetManager::Script_QueryStringFilterState( sdUIFunctionStack& stack ) {
	idStr cvar;
	stack.Pop( cvar );

	idStr value;
	stack.Pop( value );

	bool testValue = !value.IsEmpty();

	for( int i = 0; i < stringFilters.Num(); i++ ) {
		const serverStringFilter_t& filter = stringFilters[ i ];
		if( filter.cvar.Icmp( cvar.c_str() ) == 0 && ( !testValue || filter.value.Icmp( value ) == 0 ) ) {
			stack.Push( filter.state );
			return;
		}
	}

	stack.Push( SFS_DONTCARE );
}

/*
============
sdNetManager::Script_QueryStringFilterValue
============
*/
void sdNetManager::Script_QueryStringFilterValue( sdUIFunctionStack& stack ) {
	idStr cvar;
	stack.Pop( cvar );

	for( int i = 0; i < stringFilters.Num(); i++ ) {
		const serverStringFilter_t& filter = stringFilters[ i ];
		if( filter.cvar.Icmp( cvar.c_str() ) == 0 ) {
			stack.Push( filter.value.c_str() );
			return;
		}
	}

	stack.Push( idStr( "" ) );
}

#if !defined( SD_DEMO_BUILD )
/*
============
sdNetManager::Script_Team_ChooseContextAction
============
*/
void sdNetManager::Script_Team_ChooseContextAction( sdUIFunctionStack& stack ) {
	idStr memberName;
	stack.Pop( memberName );

	assert( activeMessage == NULL );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( members, memberName );
	if ( mate != NULL ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_IM );
		if( ims.Num() ) {
			activeMessage = ims[ 0 ];
			stack.Push( sdNetProperties::TCA_READ_IM );
			return;
		} else {
			sdNetMessage* message = mate->GetMessageQueue().GetMessages();
			if ( message != NULL ) {
				switch( message->GetType() ) {
					case sdNetMessage::MT_MEMBERSHIP_PROMOTION_TO_ADMIN:
						stack.Push( sdNetProperties::TCA_NOTIFY_ADMIN );
						activeMessage = message;
						return;
					case sdNetMessage::MT_MEMBERSHIP_PROMOTION_TO_OWNER:
						stack.Push( sdNetProperties::TCA_NOTIFY_OWNER );
						activeMessage = message;
						return;
					case sdNetMessage::MT_SESSION_INVITE:
						stack.Push( sdNetProperties::TCA_SESSION_INVITE );
						activeMessage = message;
						return;
				}
			}
		}
		stack.Push( sdNetProperties::TCA_SEND_IM );
		return;
	}

	stack.Push( sdNetProperties::TCA_NONE );
}

/*
============
sdNetManager::Script_Team_IsMember
============
*/
void sdNetManager::Script_Team_IsMember( sdUIFunctionStack& stack ) {
	idStr memberName;
	stack.Pop( memberName );

	float result = 0.0f;

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	const sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( members, memberName.c_str() );

	stack.Push( mate == NULL ? 0.0f : 1.0f );
}

/*
============
sdNetManager::Script_Team_IsPendingMember
============
*/
void sdNetManager::Script_Team_IsPendingMember( sdUIFunctionStack& stack ) {
	idStr memberName;
	stack.Pop( memberName );

	float result = 0.0f;

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetPendingMemberList();
	const sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( members, memberName.c_str() );

	stack.Push( mate == NULL ? 0.0f : 1.0f );
}

/*
============
sdNetManager::Script_Team_GetIMText
============
*/
void sdNetManager::Script_Team_GetIMText( sdUIFunctionStack& stack ) {
	idStr userName;
	stack.Pop( userName );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();

	if ( sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( members, userName ) ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_IM );
		if( ims.Num() ) {
			stack.Push( ims[ 0 ]->GetText() );
			return;
		}
	}
	stack.Push( L"" );
}
#endif /* !SD_DEMO_BUILD */

/*
============
sdNetManager::Script_JoinSession
============
*/
void sdNetManager::Script_JoinSession( sdUIFunctionStack& stack ) {
	assert( activeMessage != NULL );

	const netadr_t* net = reinterpret_cast< const netadr_t* >( activeMessage->GetData() );
	const char* netStr = sys->NetAdrToString( *net );
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "connect %s\n", netStr ) );
}

#if !defined( SD_DEMO_BUILD )
/*
============
sdNetManager::Script_IsFriendOnServer
============
*/
void sdNetManager::Script_IsFriendOnServer( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
	sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName.c_str() );

	if( mate == NULL ) {
		stack.Push( false );
		return;
	}

	sdNetClientId id;
	mate->GetNetClientId( id );

	const idDict* profile = networkService->GetProfileProperties( id );
	const char* server = ( profile == NULL ) ? "" : profile->GetString( "currentServer", "0.0.0.0:0" );
	stack.Push( idStr::Cmp( server, "0.0.0.0:0" ) != 0 );
}

/*
============
sdNetManager::Script_FollowFriendToServer
============
*/
void sdNetManager::Script_FollowFriendToServer( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
	sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName.c_str() );
	if( mate == NULL ) {
		return;
	}

	sdNetClientId id;
	mate->GetNetClientId( id );

	const idDict* profile = networkService->GetProfileProperties( id );

	const char* server = ( profile == NULL ) ? "0.0.0.0:0" : profile->GetString( "currentServer", "0.0.0.0:0" );
	if( idStr::Cmp( server, "0.0.0.0:0" ) != 0 ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "connect %s\n", server ) );
	}
}

/*
============
sdNetManager::Script_IsMemberOnServer
============
*/
void sdNetManager::Script_IsMemberOnServer( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	sdNetTeamMember* member = networkService->GetTeamManager().FindMember( members, friendName.c_str() );
	if( member == NULL ) {
		stack.Push( false );
		return;
	}

	sdNetClientId id;
	member->GetNetClientId( id );

	const idDict* profile = networkService->GetProfileProperties( id );
	const char* server = ( profile == NULL ) ? "" : profile->GetString( "currentServer", "0.0.0.0:0" );
	stack.Push( idStr::Cmp( server, "0.0.0.0:0" ) != 0 );
}

/*
============
sdNetManager::Script_FollowMemberToServer
============
*/
void sdNetManager::Script_FollowMemberToServer( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	sdNetTeamMember* member = networkService->GetTeamManager().FindMember( members, friendName.c_str() );

	if( member == NULL ) {
		return;
	}
	sdNetClientId id;
	member->GetNetClientId( id );

	const idDict* profile = networkService->GetProfileProperties( id );

	const char* server = ( profile == NULL ) ? "0.0.0.0:0" : profile->GetString( "currentServer", "0.0.0.0:0" );
	if( idStr::Cmp( server, "0.0.0.0:0" ) != 0 ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "connect %s\n", server ) );
	}
}
#endif /* !SD_DEMO_BUILD */

/*
============
sdNetManager::Script_IsSelfOnServer
============
*/
void sdNetManager::Script_IsSelfOnServer( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );
	sdNetUser* activeUser = networkService->GetActiveUser();

	if( activeUser == NULL ) {
		stack.Push( false );
		return;
	}

	const char* server = activeUser->GetProfile().GetProperties().GetString( "currentServer", "0.0.0.0:0" );
	stack.Push( idStr::Cmp( server, "0.0.0.0:0" ) != 0 );
}

/*
============
sdNetManager::Script_GetMotdString
============
*/
void sdNetManager::Script_GetMotdString( sdUIFunctionStack& stack ) {
	const sdNetService::motdList_t& motd = networkService->GetMotD();

	sdWStringBuilder_Heap builder;
	for( int i = 0; i < motd.Num(); i++ ) {
		builder += motd[ i ].text.c_str();
		builder += L"            ";
	}
	stack.Push( builder.c_str() );
}

/*
============
sdNetManager::Script_SetDefaultUser
============
*/
void sdNetManager::Script_SetDefaultUser( sdUIFunctionStack& stack ) {
	idStr username;
	stack.Pop( username );

	int value;
	stack.Pop( value );

	bool set = false;
	for ( int i = 0; i < networkService->NumUsers(); i++ ) {
		sdNetUser* user = networkService->GetUser( i );

		if ( !idStr::Cmp( username, user->GetRawUsername() ) ) {
			set = true;
			user->GetProfile().GetProperties().SetBool( "default", value != 0 );
			user->Save( sdNetUser::SI_PROFILE );
			break;
		}
	}

	// only clear the previous default user if we successfully set the new one
	if( set ) {
		for ( int i = 0; i < networkService->NumUsers(); i++ ) {
			sdNetUser* user = networkService->GetUser( i );

			if ( !idStr::Cmp( username, user->GetRawUsername() ) ) {
				continue;
			}
			user->GetProfile().GetProperties().SetBool( "default", 0 );
			user->Save( sdNetUser::SI_PROFILE );
		}
	}
}


/*
============
sdNetManager::Script_RefreshCurrentServers
============
*/
void sdNetManager::Script_RefreshCurrentServers( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	findServerSource_e source;
	if( !sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, source ) ) {
		gameLocal.Error( "RefreshCurrentServers: source '%i' out of range", iSource );
		stack.Push( false );
		return;
	}

	StopFindingServers( source );

	sdNetTask* task = NULL;
	idList< sdNetSession* >* netSessions = NULL;
	GetSessionsForServerSource( source, netSessions, task );

	if( netSessions == NULL ) {
		assert( 0 );
		stack.Push( false );
		return;
	}
	if( task != NULL ) {
		gameLocal.Printf( "SDNet::RefreshCurrentServers : Already scanning for servers\n" );
		stack.Push( false );
		return;
	}

	lastServerUpdateIndex = 0;

	task = networkService->GetSessionManager().RefreshSessions( *netSessions );
	if ( task == NULL ) {
		gameLocal.Printf( "SDNet::RefreshCurrentServers : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
		stack.Push( false );
		return;
	}

	stack.Push( true );
}


/*
============
sdNetManager::StopFindingServers
============
*/
void sdNetManager::StopFindingServers( findServerSource_e source ) {
	switch( source ) {
		case FS_INTERNET:
			if( findServersTask != NULL ) {
				findServersTask->Cancel( true );
				networkService->FreeTask( findServersTask );
				findServersTask = NULL;
			}
			break;
		case FS_LAN:
			if( findLANServersTask != NULL ) {
				findLANServersTask->Cancel( true );
				networkService->FreeTask( findLANServersTask );
				findLANServersTask = NULL;
			}
			break;
		case FS_HISTORY:
			if( findHistoryServersTask != NULL ) {
				findHistoryServersTask->Cancel( true );
				networkService->FreeTask( findHistoryServersTask );
				findHistoryServersTask = NULL;
			}
			break;
		case FS_FAVORITES:
			if( findFavoriteServersTask != NULL ) {
				findFavoriteServersTask->Cancel( true );
				networkService->FreeTask( findFavoriteServersTask );
				findFavoriteServersTask = NULL;
			}
			break;
	}
	lastServerUpdateIndex = 0;
}

/*
============
sdNetManager::Script_StopFindingServers
============
*/
void sdNetManager::Script_StopFindingServers( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	findServerSource_e source = static_cast< findServerSource_e >( iSource );
	if( !sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, source ) ) {
		gameLocal.Error( "StopFindingServers: source '%i' out of range", iSource );
		return;
	}
	StopFindingServers( source );
}


/*
============
sdNetManager::Script_ToggleServerFavoriteState
============
*/
void sdNetManager::Script_ToggleServerFavoriteState( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	idStr sessionName;
	stack.Pop( sessionName );

	sdNetUser* activeUser = networkService->GetActiveUser();

	idStr key = va( "favorite_%s", sessionName.c_str() );
	bool set = activeUser->GetProfile().GetProperties().GetBool( key.c_str(), "0" );
	set = !set;
	if( !set ) {
		activeUser->GetProfile().GetProperties().Delete( key.c_str() );
		common->DPrintf( "Removed '%s' as a favorite\n", sessionName.c_str() ) ;
	} else {
		common->DPrintf( "Added '%s' as a favorite\n", sessionName.c_str() ) ;
		activeUser->GetProfile().GetProperties().SetBool( key.c_str(), true );
	}
}

/*
============
sdNetManager::UpdateServer
============
*/
void sdNetManager::UpdateServer( sdUIList& list, const char* sessionName, findServerSource_e source ) {
	assert( networkService->GetActiveUser() != NULL );

	sdNetTask* task;
	idList< sdNetSession* >* netSessions;
	GetSessionsForServerSource( source, netSessions, task );

	if( findServersTask != NULL || findLANServersTask != NULL || findHistoryServersTask != NULL || findFavoriteServersTask != NULL ) {
		return;
	}

	if( netSessions != NULL ) {
		sessionHash_t::Iterator iter = hashedSessions.Find( sessionName );
		if( iter != hashedSessions.End() ) {
			if( task != NULL ) {
				task->AcquireLock();
			}

			sdNetSession* netSession = (*netSessions)[ iter->second.sessionListIndex ];

			UpdateSession( list, *netSession, iter->second.uiListIndex );

			if( task != NULL ) {
				task->ReleaseLock();
			}
		} else {
			assert( false );
		}
	} else {
		assert( false );
	}
}

#if !defined( SD_DEMO_BUILD )
/*
============
sdNetManager::Script_Team_NumAvailableIMs
============
*/
void sdNetManager::Script_Team_NumAvailableIMs( sdUIFunctionStack& stack ) {

	idStr friendName;
	stack.Pop( friendName );
	if ( friendName.IsEmpty() ) {
		stack.Push( 0 );
		return;
	}

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& friends = networkService->GetTeamManager().GetMemberList();
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( friends, friendName );
	if ( mate != NULL ) {
		idListGranularityOne< sdNetMessage* > ims;
		mate->GetMessageQueue().GetMessagesOfType( ims, sdNetMessage::MT_IM );
		stack.Push( ims.Num() );
		return;
	}
	stack.Push( 0 );
}

/*
============
sdNetManager::Script_Team_GetMemberStatus
============
*/
void sdNetManager::Script_Team_GetMemberStatus( sdUIFunctionStack& stack ) {
	idStr memberName;
	stack.Pop( memberName );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( members, memberName );
	if ( mate != NULL ) {
		stack.Push( mate->GetMemberStatus() );
		return;
	}
	stack.Push( sdNetTeamMember::MS_MEMBER );
}

/*
============
sdNetManager::ShowRanked
============
*/
bool sdNetManager::ShowRanked() const {
	for( int i = 0; i < numericFilters.Num(); i++ ) {
		const serverNumericFilter_t& filter = numericFilters[ i ];
		if( filter.type == SF_RANKED && filter.state == SFS_SHOWONLY ) {
			return true;
			break;
		}
	}
	return false;
}
#endif /* !SD_DEMO_BUILD */

/*
============
sdNetManager::Script_GetPlayerCount
============
*/
void sdNetManager::Script_GetPlayerCount( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	findServerSource_e source;
	if( !sdIntToContinuousEnum< findServerSource_e >( iSource, FS_MIN, FS_MAX, source ) ) {
		gameLocal.Error( "GetPlayerCount: source '%i' out of range", iSource );
		stack.Push( false );
		return;
	}
	idList< sdNetSession* >* sessions;
	sdNetTask* task = NULL;
	GetSessionsForServerSource( source, sessions, task );

	int count = 0;
	if( sessions != NULL ) {
		if( task != NULL ) {
			task->AcquireLock();
		}

		for( int i = 0; i < sessions->Num(); i++ ) {
			sdNetSession* netSession = (*sessions)[ i ];
			count += netSession->GetNumClients() - netSession->GetNumBotClients();
		}

		if( task != NULL ) {
			task->ReleaseLock();
		}
	}
	stack.Push( count );
}

/*
============
sdNetManager::Script_GetServerStatusString
============
*/
void sdNetManager::Script_GetServerStatusString( sdUIFunctionStack& stack ) {
	float fSource;
	stack.Pop( fSource );

	float sessionIndexFloat;
	stack.Pop( sessionIndexFloat );

	sdNetSession* netSession = GetSession( fSource, idMath::FtoiFast( sessionIndexFloat ) );

	idWStr status;

	if( netSession != NULL ) {
		const idDict& serverInfo = netSession->GetServerInfo();

		const sdGameRules* rulesInstance = gameLocal.GetRulesInstance( serverInfo.GetString( "si_rules" ) );

		if( rulesInstance != NULL ) {
			rulesInstance->GetBrowserStatusString( status, *netSession );
		}
	}

	stack.Push( status.c_str() );
}

#if !defined( SD_DEMO_BUILD )
/*
============
sdNetManager::Script_GetFriendServerIP
============
*/
void sdNetManager::Script_GetFriendServerIP( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );

	const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
	sdNetFriend* mate = networkService->GetFriendsManager().FindFriend( friends, friendName.c_str() );
	if( mate == NULL ) {
		stack.Push( "" );
		return;
	}

	sdNetClientId id;
	mate->GetNetClientId( id );
	const idDict* profile = networkService->GetProfileProperties( id );

	const char* server = ( profile == NULL ) ? "" : profile->GetString( "currentServer", "0.0.0.0:0" );
	if( idStr::Cmp( server, "0.0.0.0:0" ) != 0 ) {
		stack.Push( server );
		return;
	}
	stack.Push( "" );
}

/*
============
sdNetManager::Script_GetMemberServerIP
============
*/
void sdNetManager::Script_GetMemberServerIP( sdUIFunctionStack& stack ) {
	idStr friendName;
	stack.Pop( friendName );

	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );

	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	sdNetTeamMember* mate = networkService->GetTeamManager().FindMember( members, friendName.c_str() );
	if( mate == NULL ) {
		stack.Push( "" );
		return;
	}
	sdNetClientId id;
	mate->GetNetClientId( id );
	const idDict* profile = networkService->GetProfileProperties( id );

	const char* server = ( profile == NULL ) ? "" : profile->GetString( "currentServer", "0.0.0.0:0" );
	if( idStr::Cmp( server, "0.0.0.0:0" ) != 0 ) {
		stack.Push( server );
		return;
	}
	stack.Push( "" );
}


/*
============
sdNetManager::AnyFriendsOnServer
============
*/
bool sdNetManager::AnyFriendsOnServer( const sdNetSession& netSession ) const {
	sdStringBuilder_Heap address( netSession.GetHostAddressString() );

	int hash = serversWithFriendsHash.GenerateKey( address.c_str(), false );
	int i;
	for ( i = serversWithFriendsHash.GetFirst( hash ); i != idHashIndexUShort::NULL_INDEX; i = serversWithFriendsHash.GetNext( i ) ) {
		if( idStr::Cmp( address.c_str(), serversWithFriends[ i ] ) == 0 ) {
			return true;
		}
	}
	return false;
}

/*
============
sdNetManager::AddFriendServerToHash
============
*/
void sdNetManager::AddFriendServerToHash( const char* server ) {
	if( idStr::Cmp( server, "0.0.0.0:0" ) != 0 ) {
		int j;
		for ( j = 0; j < serversWithFriends.Num(); j++ ) {
			if( idStr::Cmp( server, serversWithFriends[ j ] ) == 0 ) {
				break;
			}
		}
		if( j >= serversWithFriends.Num() ) {
			serversWithFriendsHash.Add( serversWithFriendsHash.GenerateKey( server, false ), serversWithFriends.Append( server ) );
		}
	}
}

/*
============
sdNetManager::CacheServersWithFriends
============
*/
void sdNetManager::CacheServersWithFriends() {
	assert( networkService->GetActiveUser() != NULL );

	serversWithFriendsHash.Clear();
	serversWithFriends.SetNum( 0, false );

	{
		sdScopedLock< true > lock( networkService->GetFriendsManager().GetLock() );
		const sdNetFriendsList& friends = networkService->GetFriendsManager().GetFriendsList();
		for( int i = 0; i < friends.Num(); i++ ) {
			const sdNetFriend* mate = friends[ i ];
			if( mate->GetState() != sdNetFriend::OS_ONLINE ) {
				continue;
			}
			sdNetClientId id;
			mate->GetNetClientId( id );
			const idDict* profile = networkService->GetProfileProperties( id );

			const char* server = ( profile == NULL ) ? "0.0.0.0:0" : profile->GetString( "currentServer", "0.0.0.0:0" );
			AddFriendServerToHash( server );
		}
	}
	sdScopedLock< true > lock( networkService->GetTeamManager().GetLock() );
	const sdNetTeamMemberList& members = networkService->GetTeamManager().GetMemberList();
	for( int i = 0; i < members.Num(); i++ ) {
		const sdNetTeamMember* member = members[ i ];
		if( member->GetState() != sdNetFriend::OS_ONLINE ) {
			continue;
		}

		sdNetClientId id;
		member->GetNetClientId( id );
		const idDict* profile = networkService->GetProfileProperties( id );

		const char* server = ( profile == NULL ) ? "0.0.0.0:0" : profile->GetString( "currentServer", "0.0.0.0:0" );
		AddFriendServerToHash( server );
	}
}

/*
============
sdNetManager::Script_GetMessageTimeStamp
============
*/
void sdNetManager::Script_GetMessageTimeStamp( sdUIFunctionStack& stack ) {
	if( activeMessage == NULL ) {
		stack.Push( L"" );
		return;
	}

	const sysTime_t& time = activeMessage->GetTimeStamp();

	sdNetProperties::FormatTimeStamp( time, tempWStr );
	stack.Push( tempWStr.c_str() );
}

/*
============
sdNetManager::Script_ClearActiveMessage
============
*/
void sdNetManager::Script_ClearActiveMessage( sdUIFunctionStack& stack ) {
	assert( activeMessage != NULL );
	activeMessage = NULL;
}

/*
============
sdNetManager::Script_GetServerInviteIP
============
*/
void sdNetManager::Script_GetServerInviteIP( sdUIFunctionStack& stack ) {
	assert( activeMessage != NULL );
	netadr_t net = *reinterpret_cast< const netadr_t* >( activeMessage->GetData() );
	const char* netStr = sys->NetAdrToString( net );

	stack.Push( netStr );
}
#endif /* !SD_DEMO_BUILD */


/*
============
sdNetManager::Script_AddUnfilteredSession
============
*/
void sdNetManager::Script_AddUnfilteredSession( sdUIFunctionStack& stack ) {
	idStr address;
	stack.Pop( address );

	netadr_t addr;
	sys->StringToNetAdr( address.c_str(), &addr, false );
	if( addr.type != NA_BAD ) {
		unfilteredSessions.AddUnique( addr );
	}
}


/*
============
sdNetManager::Script_ClearUnfilteredSessions
============
*/
void sdNetManager::Script_ClearUnfilteredSessions( sdUIFunctionStack& stack ) {
	unfilteredSessions.SetNum( 0, false );
}

/*
============
sdNetManager::Script_FormatSessionInfo
============
*/
void sdNetManager::Script_FormatSessionInfo( sdUIFunctionStack& stack ) {
	idStr addr;
	stack.Pop( addr );

	for( int i = 0; i < sessions.Num(); i++ ) {
		const sdNetSession& netSession = *sessions[ i ];
		if( addr.Cmp( netSession.GetHostAddressString() ) == 0 ) {
			const idDict& serverInfo = netSession.GetServerInfo();

			sdWStringBuilder_Heap builder;

			builder += va( L"%hs\n", serverInfo.GetString( "si_name" ) );

			builder += va( L"%ls: ", common->LocalizeText( "guis/mainmenu/mapname" ).c_str() );

			// Map name
			if ( const idDict* mapInfo = GetMapInfo( netSession ) ) {
				idStr prettyName;
				prettyName = mapInfo != NULL ? mapInfo->GetString( "pretty_name", serverInfo.GetString( "si_map" ) ) : "";
				prettyName.StripFileExtension();
				builder += va( L"%hs\n", prettyName.c_str() );
			} else {
				builder += va( L"%hs\n", serverInfo.GetString( "si_map" ) );
			}

			builder += va( L"%ls: ", common->LocalizeText( "guis/mainmenu/gametype" ).c_str() );

			const char* fsGame = serverInfo.GetString( "fs_game" );

			if( !*fsGame ) {
				const char* siRules = serverInfo.GetString( "si_rules" );
				idWStr gameType;
				GetGameType( siRules, gameType );
				builder += gameType.c_str();
			} else {
				builder += va( L"%hs", fsGame );
			}
			
			builder += L"\n";

			builder += va( L"%ls: ", common->LocalizeText( "guis/mainmenu/time" ).c_str() );

			if( netSession.GetGameState() & sdGameRules::PGS_WARMUP ) {
				builder += common->LocalizeText( "guis/mainmenu/server/warmup" ).c_str();
			} else if( netSession.GetGameState() & sdGameRules::PGS_REVIEWING ) {
				builder += common->LocalizeText( "guis/mainmenu/server/reviewing" ).c_str();
			} else if( netSession.GetGameState() & sdGameRules::PGS_LOADING ) {
				builder += common->LocalizeText( "guis/mainmenu/server/loading" ).c_str();
			} else {
				idWStr::hmsFormat_t format;
				format.showZeroSeconds = false;
				format.showZeroMinutes = true;
				builder += idWStr::MS2HMS( netSession.GetSessionTime(), format );
			}
			builder += L"\n";

			builder += va( L"%ls: ", common->LocalizeText( "guis/mainmenu/players" ).c_str() );

			// Client Count
			int numBots = netSession.GetNumBotClients();
			if( numBots == 0 ) {
				builder += va( L"%d/%d\n", netSession.GetNumClients(), serverInfo.GetInt( "si_maxPlayers" ) );
			} else {
				builder += va( L"%d/%d (%d)\n", netSession.GetNumClients(), serverInfo.GetInt( "si_maxPlayers" ), numBots );
			}

			// Ping
			builder += va( L"%ls: %d", common->LocalizeText( "guis/game/scoreboard/ping" ).c_str(), netSession.GetPing() );

			stack.Push( builder.c_str() );
			return;
		}
	}
	idWStrList args( 1 );
	args.Append( va( L"%hs", addr.c_str() ) );
	stack.Push( common->LocalizeText( "guis/mainmenu/serveroffline", args ).c_str() );
	return;
}

/*
============
sdNetManager::Script_CancelActiveTask
============
*/
void sdNetManager::Script_CancelActiveTask( sdUIFunctionStack& stack ) {
	assert( activeTask.task != NULL );
	if( activeTask.task == NULL ) {
		return;
	}
	activeTask.Cancel();
}

#if !defined( SD_DEMO_BUILD )
/*
============
sdNetManager::Script_UnloadMessageHistory
============
*/
void sdNetManager::Script_UnloadMessageHistory( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	idStr username;
	stack.Pop( username );

	sdNetProperties::messageHistorySource_e source;
	if( sdIntToContinuousEnum< sdNetProperties::messageHistorySource_e >( iSource, sdNetProperties::MHS_MIN, sdNetProperties::MHS_MAX, source ) ) {
		idStr rawUserName;

		sdNetMessageHistory* history = sdNetProperties::GetMessageHistory( source, username.c_str(), rawUserName );

		if( history != NULL ) {
			if( history->IsLoaded() ) {
				history->Store();
				history->Unload();
			}
		}
	}
}

/*
============
sdNetManager::Script_LoadMessageHistory
============
*/
void sdNetManager::Script_LoadMessageHistory( sdUIFunctionStack& stack ) {
	assert( networkService->GetActiveUser() != NULL );

	int iSource;
	stack.Pop( iSource );

	idStr username;
	stack.Pop( username );

	sdNetProperties::messageHistorySource_e source;
	if( sdIntToContinuousEnum< sdNetProperties::messageHistorySource_e >( iSource, sdNetProperties::MHS_MIN, sdNetProperties::MHS_MAX, source ) ) {
		idStr rawUserName;

		sdNetMessageHistory* history = sdNetProperties::GetMessageHistory( source, username.c_str(), rawUserName );

		if( history != NULL && !history->IsLoaded() ) {
			idStr path;
			sdNetProperties::GenerateMessageHistoryFileName( networkService->GetActiveUser()->GetRawUsername(), rawUserName.c_str(), path );
			history->Load( path.c_str() );
		}
	}
}

/*
============
sdNetManager::Script_AddToMessageHistory
============
*/
void sdNetManager::Script_AddToMessageHistory( sdUIFunctionStack& stack ) {
	int iSource;
	stack.Pop( iSource );

	idStr username;
	stack.Pop( username );

	idStr fromUser;
	stack.Pop( fromUser );

	idWStr message;
	stack.Pop( message );

	bool success = false;

	sdNetProperties::messageHistorySource_e source;
	if( sdIntToContinuousEnum< sdNetProperties::messageHistorySource_e >( iSource, sdNetProperties::MHS_MIN, sdNetProperties::MHS_MAX, source ) ) {
		idStr rawUserName;

		sdNetMessageHistory* history = sdNetProperties::GetMessageHistory( source, username.c_str(), rawUserName );

		if( history != NULL ) {
			assert( history->IsLoaded() );
			const sdNetUser* activeUser = networkService->GetActiveUser();

			tempWStr = va( L"%hs: %ls", fromUser.c_str(), message.c_str() );
			history->AddEntry( tempWStr.c_str() );
			history->Store();
		} else {
			gameLocal.Warning( "Script_AddToMessageHistory: Could not find '%s'", username.c_str() );
		}
	}
	stack.Push( success );
}

/*
============
sdNetManager::Script_GetUserNamesForKey
============
*/
void sdNetManager::Script_GetUserNamesForKey( sdUIFunctionStack& stack ) {
	idStr key;
	stack.Pop( key );

	if( activeTask.task != NULL ) {
		gameLocal.Printf( "GetUserNamesForKey: task pending\n" );
		stack.Push( false );
		return;
	}

	if ( !activeTask.Set( networkService->GetAccountsForLicense( retrievedAccountNames, key.c_str() ) ) ) {
		gameLocal.Printf( "SDNet::GetUserNamesForKey : failed (%d)\n", networkService->GetLastError() );
		properties.SetTaskResult( networkService->GetLastError(), declHolder.FindLocStr( va( "sdnet/error/%d", networkService->GetLastError() ) ) );
		stack.Push( false );
		return;
	}

	properties.SetTaskActive( true );
	stack.Push( true );
}

/*
============
sdNetManager::CreateRetrievedUserNameList
============
*/
void sdNetManager::CreateRetrievedUserNameList( sdUIList* list ) {
	sdUIList::ClearItems( list );
	
	for( int i = 0; i < retrievedAccountNames.Num(); i++ ) {
		sdUIList::InsertItem( list, va( L"%hs", retrievedAccountNames[ i ].c_str() ), -1, 0 );
	}
}

#endif /* !SD_DEMO_BUILD */

