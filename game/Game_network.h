// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_NETWORK_H__
#define __GAME_NETWORK_H__

#define ASYNC_BUFFER_SECURITY
const int ASYNC_MAGIC_NUMBER = 12345678;

#include "network/SnapshotState.h"

class sdReliableServerMessage : public idBitMsg {
public:
	sdReliableServerMessage( gameReliableServerMessage_t _type ) : type( _type ) {
		InitWrite( buffer, sizeof( buffer ) );
		WriteBits( type, idMath::BitsForInteger( GAME_RELIABLE_SMESSAGE_NUM_MESSAGES ) );
	}

	void Send( int clientNum = -1 ) const;

protected:
	byte				buffer[ MAX_GAME_MESSAGE_SIZE ];
	gameReliableServerMessage_t type;
};

class sdReliableClientMessage : public idBitMsg {
public:
	sdReliableClientMessage( gameReliableClientMessage_t type ) {
		InitWrite( buffer, sizeof( buffer ) );
		WriteBits( type, idMath::BitsForInteger( GAME_RELIABLE_CMESSAGE_NUM_MESSAGES ) );
	}

	void Send( void ) const {
		networkSystem->ClientSendReliableMessage( *this );
	}

protected:
	byte				buffer[ MAX_GAME_MESSAGE_SIZE ];
};

#endif // __GAME_NETWORK_H__
