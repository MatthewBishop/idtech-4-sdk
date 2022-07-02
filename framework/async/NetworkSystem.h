// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __NETWORKSYSTEM_H__
#define __NETWORKSYSTEM_H__

#ifdef _XENON

const int MAX_ASYNC_CLIENTS			= 16;

#else

const int MAX_ASYNC_CLIENTS			= 32;

#endif

/*
===============================================================================

  Network System.

===============================================================================
*/

struct clientNetworkAddress_t {
	unsigned char	ip[ 4 ];
	unsigned short	port;

	bool operator==( const clientNetworkAddress_t& rhs ) const {
		return	ip[ 0 ] == rhs.ip[ 0 ] &&
				ip[ 1 ] == rhs.ip[ 1 ] &&
				ip[ 2 ] == rhs.ip[ 2 ] &&
				ip[ 3 ] == rhs.ip[ 3 ] &&
				port == rhs.port;
	}
};

enum voiceMode_t {
	VO_GLOBAL,
	VO_TEAM,
	VO_FIRETEAM,
	VO_NUM_MODES,
};

struct sdNetClientId;
class usercmd_t;

// packed up version for deltification and compression
typedef struct {
	short			angles[2];
	unsigned short	buttons;
	signed char		forwardmove;
	signed char		rightmove;
	signed char		upmove;
} net_usercmd_t;

class idNetworkSystem {
public:
	virtual					~idNetworkSystem( void );

	virtual void			ServerSendReliableMessage( int clientNum, const idBitMsg &msg );
	virtual int				ServerGetClientPing( int clientNum );
	virtual int				ServerGetClientPrediction( int clientNum );
	virtual int				ServerGetClientTimeSinceLastPacket( int clientNum );
	virtual int				ServerGetClientTimeSinceLastInput( int clientNum );
	virtual int				ServerGetClientOutgoingRate( int clientNum );
	virtual int				ServerGetClientIncomingRate( int clientNum );
	virtual float			ServerGetClientIncomingPacketLoss( int clientNum );
	virtual void			ServerGetClientNetworkInfo( int clientNum, clientNetworkAddress_t& info );
	virtual void			ServerGetClientNetId( int clientNum, sdNetClientId& netClientId );
	virtual const usercmd_t* ServerGetClientUserCmd( int clientNum, int frameNum );
	virtual void			ServerKickClient( int clientNum, const char* reason, bool localizedReason );

//mal: allow the network to access some engine side bot functions....
	virtual int				AllocateClientSlotForBot( int maxPlayersOnServer );
	virtual int				ServerSetBotUserCommand( int clientNum, int frameNum, const usercmd_t& cmd );
	virtual int				ServerSetBotUserName( int clientNum, const char* playerName ); 
//mal: end of bot network functions

	virtual void			ClientSendReliableMessage( const idBitMsg &msg );
	virtual int				ClientGetPrediction( void );
	virtual int				ClientGetTimeSinceLastPacket( void );
	virtual int				ClientGetOutgoingRate( void );
	virtual int				ClientGetIncomingRate( void );
	virtual float			ClientGetIncomingPacketLoss( void );
	virtual const usercmd_t* ClientGetUserCmd( int clientNum, int frameNum );
	virtual void			WriteClientUserCmds( int clientNum, idBitMsg& msg );
	virtual void			ReadClientUserCmds( int clientNum, const idBitMsg& msg );
	virtual bool			IsDedicated( void );
	virtual bool			IsLANServer( void );
	virtual bool			IsActive( void );
	virtual bool			IsClient( void );

	virtual netadr_t		ClientGetServerAddress( void ) const;
	virtual netadr_t		ServerGetBoundAddress( void ) const;

	virtual void			WriteSound( short* buffer, int numSamples );
	virtual int				UpdateSound( float* buffer, int numSpeakers, int numSamples );

	virtual void			BeginLevelLoad( void );
	virtual void			EndLevelLoad( void );

	virtual void			EnableVoip( voiceMode_t mode );
	virtual void			DisableVoip( void );

	virtual int				GetLastVoiceSentTime( void );
	virtual int				GetLastVoiceReceivedTime( int clientIndex );

	virtual int				ClientGetFrameTime( void );

	virtual int				GetDemoState( int& time, int& position, int& length, int& startPosition, int& endPosition, int &cutStartMarker, int &cutEndMarker );
	virtual const char*		GetDemoName( void );

	virtual bool			CanPlayDemo( const char* fileName );

	virtual const idDict&	GetUserInfo( int clientNum );

	virtual bool			IsRankedServer( void );

	virtual void			StartSoundTest( int duration );
	virtual bool			IsSoundTestActive( void );
	virtual bool			IsSoundTestPlaybackActive( void );
	virtual float			GetSoundTestProgress( void );

	virtual voiceMode_t		GetVoiceMode( void );
};

extern idNetworkSystem *	networkSystem;

#endif /* !__NETWORKSYSTEM_H__ */
