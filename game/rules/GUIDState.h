// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GUIDSTATE_H__
#define __GUIDSTATE_H__

#include "../../sdnet/SDNet.h"

struct clientGUIDLookup_t {
	int				ip;
	int				pbid;
	sdNetClientId	clientId;
};

class sdGUIDInfo {
public:
								sdGUIDInfo( void );

	enum matchType_t {
		MT_GUID,
		MT_IP,
		MT_PB,
		MT_INVALID,
	};

	matchType_t					GetMatchType( void ) const { return matchType; }
	int							GetBanEndTime( void ) const { return banTime; }
	const char*					GetAuthGroup( void ) const { return authGroup; }
	bool						IsBanned( void );

	void						SetGUID( sdNetClientId guid ) { matchType = MT_GUID; id.clientId = guid; }
	void						SetIP( int ip ) { matchType = MT_IP; id.ip = ip; }
	void						SetPBID( int pbid ) { matchType = MT_PB; id.pbid = pbid; }
	void						SetAuthGroup( const char* group ) { authGroup = group; }
	void						SetMatch( const clientGUIDLookup_t& lookup );

	void						SetBanTime( int t ) { banTime = t; }
	void						BanForever( void ) { banTime = -1; }
	void						Ban( int length );
	void						UnBan( void ) { banTime = 0; }

	int							CurrentTime( void );

	bool						Match( const clientGUIDLookup_t& lookup );

	void						Write( idFile* file ) const;

private:
	matchType_t					matchType;
	clientGUIDLookup_t			id;
	time_t						banTime;
	idStr						authGroup;
};

class sdGUIDFile {
public:
								sdGUIDFile( void );
								~sdGUIDFile( void );

	enum banState_t {
		BS_NOT_BANNED,
		BS_TEMP_BAN,
		BS_PERM_BAN,
	};

	void						CheckForUpdates( void );
	bool						ParseEntry( idLexer& src );
	void						AuthUser( int clientNum, const clientGUIDLookup_t& lookup );
	void						BanUser( const clientGUIDLookup_t& lookup, int length );
	banState_t					CheckForBan( const clientGUIDLookup_t& lookup );
	void						WriteGUIDFile( void );

	static int					IPForString( const char* text );
	static sdNetClientId		GUIDForString( const char* text );

private:
	void						ClearGUIDStates( void );

private:
	static const char*			guidFileName;

	int							fileTimestamp;
	idList< sdGUIDInfo >		info;
};

#endif // __GUIDSTATE_H__
