// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_SDNETPROPERTIES_H__
#define __GAME_SDNETPROPERTIES_H__

#include "../guis/UserInterfaceTypes.h"
#include "../../sdnet/SDNetErrorCode.h"
#include "../../sdnet/SDNetTeamMember.h"
#include "../../framework/NotificationSystem.h"

class sdDeclLocStr;

class sdNetProperties : public sdUIPropertyHolder {
public:
	enum friendContextAction_e {
		FCA_NONE,
		FCA_RESPOND_TO_PROPOSAL,
		FCA_SEND_IM,
		FCA_READ_IM,
		FCA_RESPOND_TO_INVITE,
		FCA_BLOCKED,
		FCA_UNBLOCKED,
		FCA_SESSION_INVITE
	};

	enum teamContextAction_e {
		TCA_NONE,
		TCA_SEND_IM,
		TCA_READ_IM,
		TCA_NOTIFY_ADMIN,
		TCA_NOTIFY_OWNER,
		TCA_SESSION_INVITE,
	};

	enum usernameValidation_e {
		UV_VALID,
		UV_EMPTY_NAME,
		UV_DUPLICATE_NAME,
		UV_INVALID_NAME,
	};

	enum emailValidation_e {
		EV_VALID,
		EV_EMPTY_MAIL,
		EV_INVALID_MAIL,
		EV_CONFIRM_MISMATCH,
	};

	enum statusPriority_e {
		SP_NEW_MESSAGE,
		SP_ONSERVER,
		SP_ONLINE,
		SP_PENDING,
		SP_INVITED,
		SP_OFFLINE,
		SP_BLOCKED,
	};

	enum teamLevel_e {
		TL_OWNER,
		TL_ADMIN,
		TL_USER,
		TL_PENDING,
	};

	enum messageHistorySource_e {
		MHS_MIN = -1,
		MHS_FRIEND,
		MHS_TEAM,
		MHS_MAX,
	};
												sdNetProperties() {}
												~sdNetProperties() {}

	virtual sdUIFunctionInstance*				GetFunction( const char* name );

	virtual sdProperties::sdProperty*			GetProperty( const char* name );
	virtual sdProperties::sdProperty*			GetProperty( const char* name, sdProperties::ePropertyType type );
	virtual sdProperties::sdPropertyHandler&	GetProperties() { return properties; }

	virtual const char*							GetName() const { return "SDNetProperties"; }

	void										Init();
	void										Shutdown();
	void										UpdateProperties();

	void										SetTaskActive( bool active );
	void										SetTaskResult( sdNetErrorCode_e errorCode, const sdDeclLocStr* resultMessage );
	void										ConnectFailed() { connectFailed = connectFailed + 1.0f; }

#if !defined( SD_DEMO_BUILD )
	void										SetTeamName( const char* name ) { teamName = name; }
#endif /* !SD_DEMO_BUILD */

	void										SetNumAvailableDWServers( const int numAvailableServers ) { this->numAvailableDWServers = numAvailableServers; }
	void										SetNumAvailableLANServers( const int numAvailableServers ) { this->numAvailableLANServers = numAvailableServers; }
	void										SetNumAvailableHistoryServers( const int numAvailableServers ) { this->numAvailableHistoryServers = numAvailableServers; }
	void										SetNumAvailableFavoritesServers( const int numAvailableServers ) { this->numAvailableFavoritesServers = numAvailableServers; }
#if !defined( SD_DEMO_BUILD )
	void										SetNumPendingClanInvites( const int numPendingClanInvites ) { this->numPendingClanInvites = numPendingClanInvites; }
	void										SetTeamMemberStatus( sdNetTeamMember::memberStatus_e teamMemberStatus ) { this->teamMemberStatus = teamMemberStatus; }
#endif /* !SD_DEMO_BUILD */
	void										SetServerRefreshComplete( bool set ) { this->serverRefreshComplete = set ? 1.0f : 0.0f; }

	void										SetFindingServers( bool set ) { this->findingServers = set ? 1.0f : 0.0f; }
#if !defined( SD_DEMO_BUILD )
	void										SetInitializingTeams( bool set ) { this->initializingTeams = set ? 1.0f : 0.0f; }
	void										SetInitializingFriends( bool set ) { this->initializingFriends = set ? 1.0f : 0.0f; }
#endif /* !SD_DEMO_BUILD */

	static void									CreateUserList( sdUIList* list );

	static void									CreateServerList( sdUIList* list );
	static void									CreateServerClientsList( sdUIList* list );
	static void									UpdateServer( sdUIList* list );
#if !defined( SD_DEMO_BUILD )
	static void									CreateFriendsList( sdUIList* list );
	static void									CreateTeamList( sdUIList* list );
	static void									CreateTeamInvitesList( sdUIList* list );
	static void									CreateAchievementsList( sdUIList* list );
	static void									CreateAchievementTasksList( sdUIList* list );
	static void									CreateMessageHistoryList( sdUIList* list );
	static void									CreateRetrievedUserNameList( sdUIList* list );

	static void									GenerateMessageHistoryFileName( const char* currentUser, const char* friendName, idStr& path );
	static sdNetMessageHistory*					GetMessageHistory( messageHistorySource_e source, const char* username, idStr& friendRawUserName );
#endif /* !SD_DEMO_BUILD */

	static void									FormatTimeStamp( const sysTime_t& time, idWStr& str );

private:
	sdFloatProperty					state;
	sdFloatProperty					disconnectReason;
	sdIntProperty					disconnectString;

	sdFloatProperty					serverRefreshComplete;
	sdFloatProperty					findingServers;
#if !defined( SD_DEMO_BUILD )
	sdFloatProperty					initializingFriends;
	sdFloatProperty					initializingTeams;
#endif /* !SD_DEMO_BUILD */

	sdFloatProperty					hasActiveUser;
	sdFloatProperty					activeUserState;
	sdStringProperty				activeUsername;
#if !defined( SD_DEMO_BUILD )
	sdStringProperty				accountUsername;
#endif /* !SD_DEMO_BUILD */

	sdFloatProperty					taskActive;
	sdFloatProperty					taskErrorCode;
	sdIntProperty					taskResultMessage;

	sdFloatProperty					numAvailableDWServers;
	sdFloatProperty					numAvailableLANServers;
	sdFloatProperty					numAvailableHistoryServers;
	sdFloatProperty					numAvailableFavoritesServers;

#if !defined( SD_DEMO_BUILD )
	sdFloatProperty					numFriends;
	sdFloatProperty					numOnlineFriends;

	sdFloatProperty					numClanmates;
	sdFloatProperty					numOnlineClanmates;

	sdFloatProperty					steamActive;
	sdStringProperty				storedLicenseCode;
	sdStringProperty				storedLicenseCodeChecksum;

	sdFloatProperty					statsRequestState;
	sdFloatProperty					numPendingClanInvites;

	sdFloatProperty					hasPendingFriendEvents;
	sdFloatProperty					hasPendingTeamEvents;

	sdFloatProperty					teamMemberStatus;
	sdFloatProperty					hasAccount;

	sdStringProperty				teamName;
#endif /* !SD_DEMO_BUILD */

	sdWStringProperty				notificationText;

	sdFloatProperty					notifyExpireTime;
	sdFloatProperty					connectFailed;
	int								nextNotifyTime;

#if !defined( SD_DEMO_BUILD )
	int								nextUserUpdate;
#endif /* !SD_DEMO_BUILD */

	sdProperties::sdPropertyHandler	properties;

	idWStrList												systemNotifications;
#if !defined( SD_DEMO_BUILD )
	idList< sdnetTeamInviteNotification_t >					teamInviteNotifications;
	idList< sdnetFriendStateChangedNotification_t >			friendStateChangedNotifications;
	idList< sdnetTeamMemberStateChangedNotification_t >		teamMemberStateChangedNotifications;
	idList< sdnetFriendIMNotification_t >					friendIMNotifications;
	idList< sdnetTeamMemberIMNotification_t >				teamMemberIMNotifications;
	idList< sdnetFriendSessionInviteNotification_t >		friendSessionInviteNotifications;
	idList< sdnetTeamMemberSessionInviteNotification_t >	teamMemberSessionInviteNotifications;
	idList< sdnetTeamDissolvedNotification_t >				teamDissolvedNotifications;
	idList< sdnetTeamKickNotification_t >					teamKickNotifications;
#endif /* !SD_DEMO_BUILD */
};

#endif /* !__GAME_SDNETPROPERTIES_H__ */
