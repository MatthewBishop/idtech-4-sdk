#include "../../idlib/precompiled.h"
#pragma hdrstop
#include "../Game_local.h"

#ifdef _USE_VOICECHAT

#define MAX_VOICE_PACKET_SIZE	1022

idCVar si_voiceChat( "si_voiceChat", "1", CVAR_GAME | CVAR_BOOL | PC_CVAR_ARCHIVE | CVAR_SERVERINFO, "enables or disables voice chat through the server" );
idCVar g_voiceChatDebug( "g_voiceChatDebug", "0", CVAR_GAME | CVAR_BOOL, "display info on voicechat net traffic" );

void idMultiplayerGame::XmitVoiceData( void )
{
	idBitMsg	outMsg;
	int			count, total;
	byte		buffer[MAX_VOICE_PACKET_SIZE];
	byte		msgBuf[MAX_VOICE_PACKET_SIZE + 1];

	if( !gameLocal.serverInfo.GetBool( "si_voiceChat" ) )
	{
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );

	// Grab any new input and send up to the server
	count = soundSystem->GetVoiceData( buffer, MAX_VOICE_PACKET_SIZE );
	total = count;
	while( count )
	{
		outMsg.BeginWriting();
		outMsg.BeginReading();
		// Client side cvar
		if( cvarSystem->GetCVarBool( "s_voiceChatEcho" ) )
		{
			outMsg.WriteByte( GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT_ECHO );
		}
		else
		{
			outMsg.WriteByte( GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT );
		}
		outMsg.WriteData( buffer, count );
	
		// FIXME!!! FIXME!!! This should be unreliable - this is very bad
		networkSystem->ClientSendReliableMessage( outMsg );

		count = soundSystem->GetVoiceData( buffer, MAX_VOICE_PACKET_SIZE );
		total += count;
	}

	if( total && g_voiceChatDebug.GetBool() )
	{
		common->Printf( "VC: sent %d bytes\n", total );
	}
}

bool idMultiplayerGame::CanTalk( idPlayer *from, idPlayer *to, bool echo )
{
	if( !to )
	{
		return( false );
	}

	if( !from )
	{
		return( false );
	}

	if( from->spectating != to->spectating )
	{
		return( false );
	}

	if( to->IsPlayerMuted( from ) )
	{
		return( false );
	}

	if( gameLocal.IsTeamGame() && from->team != to->team )
	{
		return( false );
	}

	if( from == to )
	{
		return( echo );
	}

	return( true );
}

void idMultiplayerGame::ReceiveAndForwardVoiceData( int clientNum, const idBitMsg &inMsg, bool echo )
{
	assert( clientNum >= 0 && clientNum < MAX_CLIENTS );

	idBitMsg	outMsg;
	int			i;
	byte		msgBuf[MAX_VOICE_PACKET_SIZE + 2];
	idPlayer *	from;
	
	from = ( idPlayer * )gameLocal.entities[clientNum];
	if( !gameLocal.serverInfo.GetBool( "si_voiceChat" ) || !from )
	{
		return;
	}

	// Create a new packet with forwarded data
	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_UNRELIABLE_MESSAGE_VOICEDATA_SERVER );
	outMsg.WriteByte( clientNum );
	outMsg.WriteData( inMsg.GetReadData(), inMsg.GetRemainingData() );

	if( g_voiceChatDebug.GetBool() )
	{
		common->Printf( "VC: Received %d bytes, forwarding...\n", inMsg.GetRemainingData() );
	}

	// Forward to appropriate parties
	for( i = 0; i < gameLocal.numClients; i++ ) 
	{
		idPlayer* to = ( idPlayer * )gameLocal.entities[i];
		if( to && to->GetUserInfo() && to->GetUserInfo()->GetBool( "s_voiceChatReceive" ) )
		{
			if( i != gameLocal.localClientNum && CanTalk( from, to, echo ) )
			{
				gameLocal.SendUnreliableMessage( outMsg, to->entityNumber );
				if( g_voiceChatDebug.GetBool() )
				{
					common->Printf( " ... to client %d\n", i );
				}
			}
		}
	}

	// This is revolting
	// Listen servers need to manually call the receive function
	if( gameLocal.isListenServer )
	{
		// Skip over control byte
		outMsg.ReadByte();

		idPlayer* to = gameLocal.GetLocalPlayer();
		if( to->GetUserInfo()->GetBool( "s_voiceChatReceive" ) )
		{
			if( CanTalk( from, to, echo ) )
			{
				if( g_voiceChatDebug.GetBool() )
				{
					common->Printf( " ... to local client %d\n", gameLocal.localClientNum );
				}
				ReceiveAndPlayVoiceData( outMsg );
			}
		}
	}
}

void idMultiplayerGame::ReceiveAndPlayVoiceData( const idBitMsg &inMsg )
{
	int		clientNum;

	if( !gameLocal.serverInfo.GetBool( "si_voiceChat" ) )
	{
		return;
	}
	
	clientNum = inMsg.ReadByte();
	soundSystem->PlayVoiceData( clientNum, inMsg.GetReadData(), inMsg.GetRemainingData() );
	if( g_voiceChatDebug.GetBool() )
	{
		common->Printf( "VC: Playing %d bytes\n", inMsg.GetRemainingData() );
	}
}
#endif

// end
