//----------------------------------------------------------------
// GameState.cpp
//
// Copyright 2002-2005 Raven Software
//----------------------------------------------------------------

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "GameState.h"

/*
===============================================================================

rvGameState

Game state info for deathmatch, team deathmatch

===============================================================================
*/

/*
================
rvGameState::rvGameState
================
*/
rvGameState::rvGameState( bool allocPrevious ) {
	Clear();

	if( allocPrevious ) {
		previousGameState = new rvGameState( false );
	} else {
		previousGameState = NULL;
	}
	
	trackPrevious = allocPrevious;
}

/*
================
rvGameState::~rvGameState
================
*/
rvGameState::~rvGameState( void ) {
	Clear();
	delete previousGameState;
	previousGameState = NULL;
}

/*
================
rvGameState::Clear
================
*/
void rvGameState::Clear( void ) {
	currentState = INACTIVE;
	nextState = INACTIVE;
	nextStateTime = 0;
	fragLimitTimeout = 0;
}

/*
================
rvGameState::SendState
================
*/
void rvGameState::SendState( int clientNum ) {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	assert( gameLocal.isServer && trackPrevious );

	if( clientNum == -1 && (*this) == (*previousGameState) ) {
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_GAMESTATE );

	WriteState( outMsg );

	networkSystem->ServerSendReliableMessage( clientNum, outMsg );
	
	// don't update the state if we are working for a single client
	if ( clientNum == -1 ) {
		outMsg.ReadByte(); // pop off the msg ID
		ReceiveState( outMsg );
	}
}

/*
===============
rvGameState::WriteState
===============
*/
void rvGameState::WriteState( idBitMsg &msg ) {
	PackState( msg );
}

/*
================
rvGameState::SendInitialState
================
*/
void rvGameState::SendInitialState( int clientNum ) {
	rvGameState* previousState = previousGameState;

	rvGameState invalidState;

	previousGameState = &invalidState;

	SendState( clientNum );

	previousGameState = previousState;
}

/*
================
rvGameState::ReceiveState
================
*/
void rvGameState::ReceiveState( const idBitMsg& msg ) {
	UnpackState( msg );

	if ( gameLocal.localClientNum >= 0 ) {
		GameStateChanged();
	}

	(*previousGameState) = (*this);
}

/*
================
rvGameState::PackState
================
*/
void rvGameState::PackState( idBitMsg& outMsg ) {
	// for now, we only transmit 3 bytes.  If we need to sync more data, we should
	// only transmit the diff
	outMsg.WriteByte( currentState );
	outMsg.WriteByte( nextState );
	outMsg.WriteLong( nextStateTime );
}

/*
================
rvGameState::UnpackState
================
*/
void rvGameState::UnpackState( const idBitMsg& inMsg ) {
	currentState = (mpGameState_t)inMsg.ReadByte();
	nextState = (mpGameState_t)inMsg.ReadByte();
	nextStateTime = inMsg.ReadLong();
}

/*
================
rvGameState::GameStateChanged
================
*/
void rvGameState::GameStateChanged( void ) {
	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		if ( gameLocal.GetDemoState() == DEMO_PLAYING && gameLocal.IsServerDemo() && gameLocal.GetDemoFollowClient() >= 0 ) {
			player = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.GetDemoFollowClient() ] );
		}
	}
	if ( !player ) {
		gameLocal.Warning( "rvGameState::GameStateChanged() - NULL local player\n" ) ;
		return;
	}

	// Check for a currentState change
	if( currentState != previousGameState->currentState ) {
		if( currentState == WARMUP ) {
			if( gameLocal.gameType != GAME_TOURNEY ) {
				player->GUIMainNotice( common->GetLocalizedString( "#str_107706" ), true );		
			}
#ifdef _XENON
			// on Xenon remove the pre game lobby when the countdown starts
			session->SetGUI(NULL, NULL);
			player->disableHud = false;
			gameLocal.mpGame.ClearMenu();
#endif
			soundSystem->SetActiveSoundWorld( true );
			
			// reset stats on the client-side
			if( gameLocal.isClient ) {
				statManager->Init();
			}
		} else if( currentState == COUNTDOWN ) {
			if( gameLocal.gameType != GAME_TOURNEY ) {
				player->GUIMainNotice( common->GetLocalizedString( "#str_107706" ), true );		
			}
#ifdef _XENON
			// on Xenon remove the pre game lobby when the countdown starts
			session->SetGUI(NULL, NULL);
			player->disableHud = false;
			gameLocal.mpGame.ClearMenu();
#endif
			soundSystem->SetActiveSoundWorld(true);
			if( gameLocal.gameType != GAME_TOURNEY && previousGameState->currentState != INACTIVE ) {
				gameLocal.mpGame.RemoveAnnouncerSound( AS_GENERAL_PREPARE_TO_FIGHT );
				gameLocal.mpGame.RemoveAnnouncerSound( AS_GENERAL_THREE );
				gameLocal.mpGame.RemoveAnnouncerSound( AS_GENERAL_TWO );
				gameLocal.mpGame.RemoveAnnouncerSound( AS_GENERAL_ONE );

				gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_PREPARE_TO_FIGHT, gameLocal.time );
				gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_THREE, nextStateTime - 3000 );
				gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_TWO, nextStateTime - 2000 );
				gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_ONE, nextStateTime - 1000 );
			}
		} else if( currentState == GAMEON ) {
			if ( !player->vsMsgState ) {
				player->GUIMainNotice( "" );
				player->GUIFragNotice( "" );
			} else {
				player->vsMsgState = false;
			}
			if( gameLocal.gameType != GAME_TOURNEY ) {
				gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_FIGHT, gameLocal.time );
			}
			cvarSystem->SetCVarString( "ui_ready", "Not Ready" );
#ifdef _XENON
			// on Xenon remove the pre game lobby when the countdown starts
			session->SetGUI(NULL, NULL);
			player->disableHud = false;
			gameLocal.mpGame.ClearMenu();
			// Kill the loading gui if it's up
			session->KillLoadingGUI();
#endif
			soundSystem->SetActiveSoundWorld( true );
			
			// tourney time announcements are scheduled as you join/leave arenas and at arena starts
			if( gameLocal.gameType != GAME_TOURNEY ) {
				gameLocal.mpGame.ScheduleTimeAnnouncements( );
			}


			// clear stats on client
			if( gameLocal.isClient ) {
				statManager->Init();
			}
 		} else if( currentState == SUDDENDEATH ) {
#ifdef _XENON
			// on Xenon remove the pre game lobby when the countdown starts
			session->SetGUI(NULL, NULL);
			player->disableHud = false;
			gameLocal.mpGame.ClearMenu();
#endif
			soundSystem->SetActiveSoundWorld( true );
			gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_SUDDEN_DEATH, gameLocal.time );
			gameLocal.GetLocalPlayer()->GUIMainNotice( common->GetLocalizedString( "#str_104287" ) );
		} else if( currentState == GAMEREVIEW ) {
			// the game goes into (GAMEREVIEW->INACTIVE) before going into (GAMEREVIEW->WARMUP), so
			// check nextState to make sure this only gets called once
			//if( nextState == INACTIVE ) {
#ifdef _XENON
				// on Xenon remove the pre game lobby when the countdown starts
				session->SetGUI(NULL, NULL);
				player->disableHud = false;
				gameLocal.mpGame.ClearMenu();
				Live()->PlayedRound();
				if(Live()->RoundsPlayed() < cvarSystem->GetCVarInteger("si_matchRounds"))
				{
					gameLocal.mpGame.ShowStatSummary();
				}
#else
				gameLocal.mpGame.ShowStatSummary();
#endif
				soundSystem->SetActiveSoundWorld( true );
				if( gameLocal.IsTeamGame() && !player->wantSpectate ) {
					int winningTeam = gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) > gameLocal.mpGame.GetScoreForTeam( TEAM_STROGG ) ? TEAM_MARINE : TEAM_STROGG;
					if( player->team == winningTeam ) {
						gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_YOU_WIN, gameLocal.time );
					} else {
						gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_YOU_LOSE, gameLocal.time );
					}
				} else if( gameLocal.gameType != GAME_TOURNEY && !player->wantSpectate ) {
					if( player->GetRank() == 0 ) {
						gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_YOU_WIN, gameLocal.time );
					} else {
						gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_YOU_LOSE, gameLocal.time );
					}
				}

			//}
		}
	}
}

/*
================
rvGameState::Run
================
*/
void rvGameState::Run( void ) {
	if ( currentState == INACTIVE ) {

#ifdef _XENON
		if(Live()->RoundsPlayed() < cvarSystem->GetCVarInteger("si_matchRounds"))
#endif
		{
			NewState( WARMUP );
		}
	}

	if ( nextState != INACTIVE && gameLocal.time > nextStateTime ) {
		NewState( nextState );
		nextState = INACTIVE;
	}

	switch( currentState ) {
		case INACTIVE:
#ifdef _XENON
			if(Live()->RoundsPlayed() >= cvarSystem->GetCVarInteger("si_matchRounds") )
			{	
				Live()->EndMatch();
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect\n" );
			}
#endif
			break;

		case GAMEREVIEW: {
			if ( nextState == INACTIVE ) {
				// asalmon: Xenon only plays one match and then exists
				statManager->SendAllStats();
#ifndef _XENON
				nextState = NEXTGAME;
				// allow a little extra time in tourney since we have to display end brackets
				if( gameLocal.gameType == GAME_TOURNEY ) {
					nextStateTime = gameLocal.time + 5000 + (1000 * cvarSystem->GetCVarInteger( "g_gameReviewPause" ));
				} else {
					nextStateTime = gameLocal.time + (1000 * cvarSystem->GetCVarInteger( "g_gameReviewPause" ));
				}				
#else
				//get in one last stat update before shutdown
				//statManager->SendAllStats();
				nextState = NEXTGAME;
				nextStateTime = gameLocal.time + 1000 * cvarSystem->GetCVarInteger( "g_gameReviewPause" );
				if(Live()->RoundsPlayed() >= cvarSystem->GetCVarInteger("si_matchRounds"))
				{

					Live()->WriteSessionStats();
					//nextState = INACTIVE;
					//nextStateTime = gameLocal.time + 1000 * cvarSystem->GetCVarInteger( "g_gameReviewPause" );
					//cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect\n" );
					
				}
			/*	else
				{
					Live()->PickMap();
				}*/
#endif
			}
			break;
		}
		case NEXTGAME: {

			// the core will force a restart at 12 hours max
			// but it's nicer if we can wait for a game transition to perform the restart so the game is not interrupted
			// perform a restart once we are past 8 hours
			if ( networkSystem->ServerGetServerTime() > 8*60*60*1000 ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "nextMap\n" );
				return;
			}

#ifdef _XENON
			if(Live()->RoundsPlayed() >= cvarSystem->GetCVarInteger("si_matchRounds"))
			{
				NewState(INACTIVE);
				return;
			}
#endif

			if ( nextState == INACTIVE ) {
				// game rotation, new map, gametype etc.
				// only cycle in tourney if tourneylimit is higher than the specified value
				if( gameLocal.gameType != GAME_TOURNEY || ((rvTourneyGameState*)this)->GetTourneyCount() >= gameLocal.serverInfo.GetInt( "si_tourneyLimit" ) ) {
					// whether we switch to the next map or not, reset the tourney count
					if( gameLocal.gameType == GAME_TOURNEY ) {
						((rvTourneyGameState*)this)->SetTourneyCount( 0 );
					}


					if ( gameLocal.NextMap() ) {
						cmdSystem->BufferCommandText( CMD_EXEC_NOW, "serverMapRestart\n" );
						return;
					}
				}

				NewState( WARMUP );

				// put everyone back in from endgame spectate
				for ( int i = 0; i < gameLocal.numClients; i++ ) {
					idEntity *ent = gameLocal.entities[ i ];
					if ( ent && ent->IsType( idPlayer::GetClassType() ) ) {
						if ( !static_cast< idPlayer * >( ent )->wantSpectate ) {
							gameLocal.mpGame.CheckRespawns( static_cast<idPlayer *>( ent ) );
						}
					}
				}
			}
			break;
		}
		case WARMUP: {
			// check to see if we actually want to do a warmup, or if we fall through

//RAVEN BEGIN
//asalmon: Live has its own rules for ending warm up
#ifdef _XENON
			if(!Live()->RollCall())
			{
				break;
			}
			
#endif
//RAVEN END
			if( !gameLocal.serverInfo.GetBool( "si_warmup" ) && gameLocal.gameType != GAME_TOURNEY ) {
				// tourney always needs a warmup, to ensure that at least 2 players get seeded for the tournament.
				NewState( GAMEON );
			} else if ( gameLocal.mpGame.AllPlayersReady() ) {			
				NewState( COUNTDOWN );
				nextState = GAMEON;
				nextStateTime = gameLocal.time + 1000 * gameLocal.serverInfo.GetInt( "si_countDown" );
			}
			break;
		}
		case COUNTDOWN: {
			break;
		}
	}
}

/*
================
rvGameState::NewState
================
*/
void rvGameState::NewState( mpGameState_t newState ) {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];
	int			i;

	assert( (newState != currentState) && gameLocal.isServer );
	
	switch( newState ) {
		case WARMUP: {
			//	asalmon: start the stat manager as soon as the game starts
			statManager->Init();
			statManager->BeginGame();

			// if shuffle is on, shuffle the teams around
			if( gameLocal.IsTeamGame() && gameLocal.serverInfo.GetBool( "si_shuffle" ) ) {
				gameLocal.mpGame.ShuffleTeams();
			}

			// allow damage in warmup
			//gameLocal.mpGame.EnableDamage( false );

			//asalmon: clear out lingering team scores.
			gameLocal.mpGame.ClearTeamScores();

			if( gameLocal.gameType != GAME_TOURNEY ) {
				for( i = 0; i < gameLocal.numClients; i++ ) {
					idEntity *ent = gameLocal.entities[ i ];
					if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
						continue;
					}
					((idPlayer*)ent)->JoinInstance( 0 );
				}
			}

			break;
		}
		case GAMEON: {
			// allow damage in warmup
			//gameLocal.mpGame.EnableDamage( true );
			gameLocal.LocalMapRestart();

			// asalmon: reset the stats when the warmup period is over
			statManager->Init();
			statManager->BeginGame();

			outMsg.Init( msgBuf, sizeof( msgBuf ) );
			outMsg.WriteByte( GAME_RELIABLE_MESSAGE_RESTART );
			outMsg.WriteBits( 0, 1 );
			networkSystem->ServerSendReliableMessage( -1, outMsg );

			gameLocal.mpGame.SetMatchStartedTime( gameLocal.time );

			fragLimitTimeout = 0;

			// write server initial reliable messages to give everyone new base
			for( i = 0; i < MAX_CLIENTS; i++ ) {
				// dont send this to server - we have all the data already and this will
				// just trigger extra gamestate detection
				if ( gameLocal.entities[ i ] && i != gameLocal.localClientNum ) {
					gameLocal.mpGame.ServerWriteInitialReliableMessages( i );
				}
			}
			

			for( i = 0; i < gameLocal.numClients; i++ ) {
				idEntity *ent = gameLocal.entities[ i ];
				if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
					continue;
				}
				idPlayer *p = static_cast<idPlayer *>( ent );
				p->SetLeader( false ); // don't carry the flag from previous games	
				gameLocal.mpGame.SetPlayerScore( p, 0 );
				gameLocal.mpGame.SetPlayerTeamScore( p, 0 );

				// in normal gameplay modes, spawn the player in.  For tourney, the tourney manager handles spawning
				if( gameLocal.gameType != GAME_TOURNEY ) {
					p->JoinInstance( 0 );
					// in team games, let UserInfoChanged() spawn people on the right teams
					if( !gameLocal.IsTeamGame() || p->team != -1 ) {
						p->ServerSpectate( static_cast<idPlayer *>(ent)->wantSpectate );
					}
				}
			}

			gameLocal.mpGame.ClearTeamScores();

			cvarSystem->SetCVarString( "ui_ready", "Not Ready" );
			gameLocal.mpGame.switchThrottle[ 1 ] = 0;	// passby the throttle
			break;
		}
		case GAMEREVIEW: {
			statManager->EndGame();

			//statManager->DebugPrint();
			nextState = INACTIVE;	// used to abort a game. cancel out any upcoming state change

			for( i = 0; i < gameLocal.numClients; i++ ) {
				idEntity *ent = gameLocal.entities[ i ];
				// RAVEN BEGIN
				// jnewquist: Use accessor for static class type 
				if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
					// RAVEN END
					continue;
				}
				static_cast< idPlayer *>( ent )->forcedReady = false;
				static_cast<idPlayer *>(ent)->ServerSpectate( true );
			}
			break;
		}
		case SUDDENDEATH: {
			gameLocal.mpGame.PrintMessageEvent( -1, MSG_SUDDENDEATH );
			//unmark all leaders, so we make sure we only let the proper people respawn
			for( i = 0; i < gameLocal.numClients; i++ ) {
				idEntity *ent = gameLocal.entities[ i ];
				if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
					continue;
				}
				idPlayer *p = static_cast<idPlayer *>( ent );
				p->SetLeader( false ); // don't carry the flag from previous games	
			}

			// only restart in team games if si_suddenDeathRestart is set.
			if( !gameLocal.IsTeamGame() || gameLocal.serverInfo.GetBool( "si_suddenDeathRestart" ) ) {
				gameLocal.LocalMapRestart();
			}

			// Mark everyone tied for the lead as leaders
			i = 0;
			idPlayer* leader = gameLocal.mpGame.GetRankedPlayer( i );
			if( leader ) {
				int highScore = gameLocal.mpGame.GetScore( leader );
				while( leader ) {
					if( gameLocal.mpGame.GetScore( leader ) < highScore ) {
						break;
					}
					leader->SetLeader( true );
					leader = gameLocal.mpGame.GetRankedPlayer( ++i );
				}
			}

			if ( gameLocal.gameType == GAME_DM ) {
				//send all the non-leaders to spectatormode?
				for( i = 0; i < gameLocal.numClients; i++ ) {
					idEntity *ent = gameLocal.entities[ i ];
					// RAVEN BEGIN
					// jnewquist: Use accessor for static class type 
					if ( !ent || !ent->IsType( idPlayer::GetClassType() ) ) {
						// RAVEN END
						continue;
					}
					if ( static_cast< idPlayer *>( ent )->IsLeader() ) {
 						static_cast<idPlayer *>(ent)->ServerSpectate( false );
						continue;
					}
					static_cast< idPlayer *>( ent )->forcedReady = false;
					static_cast<idPlayer *>(ent)->ServerSpectate( true );
				}
			}
			
			break;
		}
		default: {
			break;
		}
	}

	currentState = newState;
}

/*
================
rvGameState::ClientDisconnect
================
*/
void rvGameState::ClientDisconnect( idPlayer* player ) {
	return;
}

/*
================
rvGameState::Spectate
================
*/
void rvGameState::Spectate( idPlayer* player ) {
	if( player->spectating && player->wantSpectate ) {
		gameLocal.mpGame.ClearFrags( player->entityNumber );
	}
	return;
}

/*
================
rvGameState::operator==
================
*/
bool rvGameState::operator==( const rvGameState& rhs ) const {
	return	(
		( currentState == rhs.currentState ) &&
		( nextState	== rhs.nextState ) &&
		( nextStateTime == rhs.nextStateTime ) 
		);
}

/*
================
rvGameState::operator!=
================
*/
bool rvGameState::operator!=( const rvGameState& rhs ) const {
	return	(
		( currentState != rhs.currentState ) ||
		( nextState	!= rhs.nextState ) ||
		( nextStateTime != rhs.nextStateTime ) 
		);
}

/*
================
rvGameState::operator=
================
*/
rvGameState& rvGameState::operator=( const rvGameState& rhs ) {
	currentState = rhs.currentState;
	nextState = rhs.nextState;
	nextStateTime = rhs.nextStateTime;
	return (*this);
}

/*
===============
rvGameState::WriteNetworkInfo
===============
*/
void rvGameState::WriteNetworkInfo( idFile *file, int clientNum ) {
	idBitMsg	msg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];
	
	msg.Init( msgBuf, sizeof( msgBuf ) );
	msg.BeginWriting();
	WriteState( msg );
	file->WriteInt( msg.GetSize() );
	file->Write( msg.GetData(), msg.GetSize() );
}

/*
===============
rvGameState::ReadNetworkInfo
===============
*/
void rvGameState::ReadNetworkInfo( idFile *file, int clientNum ) {
	idBitMsg	msg;
	byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];
	int			size;

	file->ReadInt( size );
	msg.Init( msgBuf, size );
	msg.SetSize( size );
	file->Read( msg.GetData(), size );
	ReceiveState( msg );
}

/*
===============================================================================

rvDMGameState

Game state info for DM

===============================================================================
*/
rvDMGameState::rvDMGameState( bool allocPrevious ) : rvGameState( allocPrevious ) {
	trackPrevious = allocPrevious;
}

void rvDMGameState::Run( void ) {
	idPlayer* player = NULL;

	rvGameState::Run();
	
	switch( currentState ) {
		case GAMEON: {
			player = gameLocal.mpGame.FragLimitHit();

			bool tiedForFirst = false;
			idPlayer* first = gameLocal.mpGame.GetRankedPlayer( 0 );
			idPlayer* second = gameLocal.mpGame.GetRankedPlayer( 1 );

			if( player == NULL ) {
				if( first && second && gameLocal.mpGame.GetScore( first ) == gameLocal.mpGame.GetScore( second ) ) {
					tiedForFirst = true;
				}
			}

			if ( player ) {
				// delay between detecting frag limit and ending game. let the death anims play
				if ( !fragLimitTimeout ) {
					common->DPrintf( "enter FragLimit timeout, player %d is leader\n", player->entityNumber );
					fragLimitTimeout = gameLocal.time + FRAGLIMIT_DELAY;
				}
				if ( gameLocal.time > fragLimitTimeout ) {
					NewState( GAMEREVIEW );

					gameLocal.mpGame.PrintMessageEvent( -1, MSG_FRAGLIMIT, player->entityNumber );

				}
			} else if ( fragLimitTimeout ) {
				// frag limit was hit and cancelled. means the two teams got even during FRAGLIMIT_DELAY
				// enter sudden death, the next frag leader will win
				//
				// jshepard: OR it means that the winner killed himself during the fraglimit delay, and the
				// game needs to roll on.
				if( first && second && (gameLocal.mpGame.GetScore( first ) == gameLocal.mpGame.GetScore( second )) )	{
					//this is a tie...
					if( gameLocal.mpGame.GetScore( first ) >= gameLocal.serverInfo.GetInt( "si_fragLimit" ) )	{
						 //and it must be tied at fraglimit, so sudden death.
						NewState( SUDDENDEATH ); 
					}
				}
				//otherwise, just keep playing as normal.
				fragLimitTimeout = 0;
				
			} else if ( gameLocal.mpGame.TimeLimitHit() ) {
				gameLocal.mpGame.PrintMessageEvent( -1, MSG_TIMELIMIT );
				if( tiedForFirst ) {
					// if tied at timelimit hit, goto sudden death
					fragLimitTimeout = 0;
					NewState( SUDDENDEATH );
				} else {
					// or just end the game
					NewState( GAMEREVIEW );
				}
			} else if( tiedForFirst && gameLocal.serverInfo.GetInt( "si_fragLimit" ) > 0 && gameLocal.mpGame.GetScore( first ) >= gameLocal.serverInfo.GetInt( "si_fragLimit" ) ) {
				// check for the rare case that two players both hit the fraglimit the same frame
				// two people tied at fraglimit, advance to sudden death after a delay
				fragLimitTimeout = gameLocal.time + FRAGLIMIT_DELAY;
			}
	
			break;			 
		}

		case SUDDENDEATH: {
			player = gameLocal.mpGame.FragLeader();

			if ( player ) {
				if ( !fragLimitTimeout ) {
					common->DPrintf( "enter sudden death FragLeader timeout, player %d is leader\n", player->entityNumber );
					fragLimitTimeout = gameLocal.time + FRAGLIMIT_DELAY;
				}
				if ( gameLocal.time > fragLimitTimeout ) {
					NewState( GAMEREVIEW );
					gameLocal.mpGame.PrintMessageEvent( -1, MSG_FRAGLIMIT, player->entityNumber );
				}
			} else if ( fragLimitTimeout ) {
				gameLocal.mpGame.PrintMessageEvent( -1, MSG_HOLYSHIT );
				fragLimitTimeout = 0;
			}
			break;
		}

	}
}

/*
===============================================================================

rvTeamDMGameState

Game state info for Team DM

===============================================================================
*/
rvTeamDMGameState::rvTeamDMGameState( bool allocPrevious ) : rvGameState( allocPrevious ) {
	trackPrevious = allocPrevious;
}

void rvTeamDMGameState::Run( void ) {
	rvGameState::Run();

	switch( currentState ) {
		case GAMEON: {
			int team = ( ( gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) >= gameLocal.serverInfo.GetInt( "si_fragLimit" ) ) ? TEAM_MARINE : ( ( gameLocal.mpGame.GetScoreForTeam( TEAM_STROGG ) >= gameLocal.serverInfo.GetInt( "si_fragLimit" ) ) ? TEAM_STROGG : -1 ) );
			if( gameLocal.serverInfo.GetInt( "si_fragLimit" ) <= 0 ) {
				// no fraglimit
				team = -1;
			}
			bool tiedForFirst = gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) == gameLocal.mpGame.GetScoreForTeam( TEAM_STROGG );
			if ( team >= 0 && !tiedForFirst ) {
				if ( !fragLimitTimeout ) {
					common->DPrintf( "enter FragLimit timeout, team %d is leader\n", team );
					fragLimitTimeout = gameLocal.time + FRAGLIMIT_DELAY;
				}
				if ( gameLocal.time > fragLimitTimeout ) {
					NewState( GAMEREVIEW );

					gameLocal.mpGame.PrintMessageEvent( -1, MSG_FRAGLIMIT, team );

				}
			} else if ( fragLimitTimeout ) {
				// frag limit was hit and cancelled. means the two teams got even during FRAGLIMIT_DELAY
				// enter sudden death, the next frag leader will win
				//
				// jshepard: OR it means that the winner killed himself during the fraglimit delay, and the
				// game needs to roll on.
				if( tiedForFirst )	{
					//this is a tie
					if( gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) >= gameLocal.serverInfo.GetInt( "si_fragLimit" ) )	{
						//and it's tied at the fraglimit.
						NewState( SUDDENDEATH ); 
					}
					//not a tie, game on.
					fragLimitTimeout = 0;
				}
			} else if ( gameLocal.mpGame.TimeLimitHit() ) {
				gameLocal.mpGame.PrintMessageEvent( -1, MSG_TIMELIMIT );
				if( tiedForFirst ) {
					// if tied at timelimit hit, goto sudden death
					fragLimitTimeout = 0;
					NewState( SUDDENDEATH );
				} else {
					// or just end the game
					NewState( GAMEREVIEW );
				}
			} else if( tiedForFirst && team >= 0 ) {
				// check for the rare case that two teams both hit the fraglimit the same frame
				// two people tied at fraglimit, advance to sudden death after a delay
				fragLimitTimeout = gameLocal.time + FRAGLIMIT_DELAY;
			}
			break;
		}

		case SUDDENDEATH: {
			int team = gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) > gameLocal.mpGame.GetScoreForTeam( TEAM_STROGG ) ? TEAM_MARINE : TEAM_STROGG;
			bool tiedForFirst = false;
			if( gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) == gameLocal.mpGame.GetScoreForTeam( TEAM_STROGG ) ) {
				team = -1;
				tiedForFirst = true;
			}

			if ( team >= 0 && !tiedForFirst ) {
				if ( !fragLimitTimeout ) {
					common->DPrintf( "enter sudden death FragLeader timeout, team %d is leader\n", team );
					fragLimitTimeout = gameLocal.time + FRAGLIMIT_DELAY;
				}
				if ( gameLocal.time > fragLimitTimeout ) {
					NewState( GAMEREVIEW );
					gameLocal.mpGame.PrintMessageEvent( -1, MSG_FRAGLIMIT, team );
				}
			} else if ( fragLimitTimeout ) {
				gameLocal.mpGame.PrintMessageEvent( -1, MSG_HOLYSHIT );
				fragLimitTimeout = 0;
			} 

			break;
		}
	}
}

/*
===============================================================================

rvCTFGameState

Game state info for CTF

===============================================================================
*/

/*
================
rvCTFGameState::rvCTFGameState
================
*/
rvCTFGameState::rvCTFGameState( bool allocPrevious ) : rvGameState( false ) {
	Clear();

	if( allocPrevious ) {
		previousGameState = new rvCTFGameState( false );
	} else {
		previousGameState = NULL;
	}

	trackPrevious = allocPrevious;
}

/*
================
rvCTFGameState::Clear
================
*/
void rvCTFGameState::Clear( void ) {
	rvGameState::Clear();

	// mekberg: clear previous game state.
	if ( previousGameState ) {
		previousGameState->Clear( );
	}		

	for( int i = 0; i < TEAM_MAX; i++ ) {
		flagStatus[ i ].state = FS_AT_BASE;
		flagStatus[ i ].clientNum = -1;
	}

	for( int i = 0; i < MAX_AP; i++ ) {
		apState[ i ] = AS_NEUTRAL;
	}
}

/*
================
rvCTFGameState::SendState
================
*/
void rvCTFGameState::SendState( int clientNum ) {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	assert( gameLocal.isServer && trackPrevious && IsType( rvCTFGameState::GetClassType() ) );

	if( clientNum == -1 && (rvCTFGameState&)(*this) == (rvCTFGameState&)(*previousGameState) ) {
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_GAMESTATE );

	WriteState( outMsg );

	networkSystem->ServerSendReliableMessage( clientNum, outMsg );

	// don't update the state if we are working for a single client
	if ( clientNum == -1 ) {
		outMsg.ReadByte(); // pop off the msg ID
		ReceiveState( outMsg );
	}
}

/*
===============
rvCTFGameState::WriteState
===============
*/
void rvCTFGameState::WriteState( idBitMsg &msg ) {
	// send off base info
	rvGameState::PackState( msg );
	// add CTF info
	PackState( msg );
}

/*
================
rvCTFGameState::ReceiveState
================
*/
void rvCTFGameState::ReceiveState( const idBitMsg& msg ) {
	assert( IsType( rvCTFGameState::GetClassType() ) );

	rvGameState::UnpackState( msg );
	UnpackState( msg );

	if ( gameLocal.localClientNum >= 0 ) {
		GameStateChanged();
	}

	(rvCTFGameState&)(*previousGameState) = (rvCTFGameState&)(*this);
}

/*
================
rvCTFGameState::PackState
================
*/
void rvCTFGameState::PackState( idBitMsg& outMsg ) {
	// use indexing to pack in info
	int index = 0;

	for( int i = 0; i < TEAM_MAX; i++ ) {
		if( flagStatus[ i ] != ((rvCTFGameState*)previousGameState)->flagStatus[ i ] ) {
			outMsg.WriteByte( index );
			outMsg.WriteByte( flagStatus[ i ].state );
			outMsg.WriteByte( flagStatus[ i ].clientNum );
		}
		index++;
	}

	for( int i = 0; i < MAX_AP; i++ ) {
		if( apState[ i ] != ((rvCTFGameState*)previousGameState)->apState[ i ] ) {
			outMsg.WriteByte( index );
			outMsg.WriteByte( apState[ i ] );
		}
		index++;
	}
}

/*
================
rvCTFGameState::UnpackState
================
*/
void rvCTFGameState::UnpackState( const idBitMsg& inMsg ) {
	while( inMsg.GetRemainingData() ) {
		int index = inMsg.ReadByte();

		if( index >= 0 && index < TEAM_MAX ) {
			flagStatus[ index ].state = (flagState_t)inMsg.ReadByte();
			flagStatus[ index ].clientNum = inMsg.ReadByte();
		} else if( index >= TEAM_MAX && index < ( TEAM_MAX + MAX_AP ) ) {
			apState[ index - TEAM_MAX ] = (apState_t)inMsg.ReadByte();
		} else {
			gameLocal.Error( "rvCTFGameState::UnpackState() - Unknown data identifier '%d'\n", index );
		}
	}
}

/*
================
rvCTFGameState::SendInitialState
================
*/
void rvCTFGameState::SendInitialState( int clientNum ) {
	assert( IsType( rvCTFGameState::GetClassType() ) );

	rvCTFGameState* previousState = (rvCTFGameState*)previousGameState;

	rvCTFGameState invalidState;

	previousGameState = &invalidState;

	SendState( clientNum );

	previousGameState = previousState;
}

/*
================
rvCTFGameState::GameStateChanged
================
*/
void rvCTFGameState::GameStateChanged( void ) {
	// detect any base state changes
	rvGameState::GameStateChanged();

	// CTF specific stuff
	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		if ( gameLocal.GetDemoState() == DEMO_PLAYING && gameLocal.IsServerDemo() && gameLocal.GetDemoFollowClient() >= 0 ) {
			player = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.GetDemoFollowClient() ] );
		}
	}
	if ( !player ) {
		gameLocal.Warning( "rvCTFGameState::GameStateChanged() - NULL local player\n" ) ;
		return;
	}

	bool noSounds = false;

	for( int i = 0; i < TEAM_MAX; i++ ) {
		if( flagStatus[ i ] == ((rvCTFGameState*)previousGameState)->flagStatus[ i ] ) {
			continue;
		}

		// don't play flag messages when flag state changes as a result of the gamestate changing
		if( currentState != ((rvCTFGameState*)previousGameState)->currentState && ((rvCTFGameState*)previousGameState)->currentState != INACTIVE ) {
			continue;
		}

		if ( ((rvCTFGameState*)previousGameState)->currentState == INACTIVE ) {
			noSounds = true;
		}

		// flagTeam - used to tell the HUD which flag to update
		int flagTeam = i;

		// in one flag CTF flagTeam is set to whichever team has the flag
		if( gameLocal.gameType == GAME_1F_CTF || gameLocal.gameType == GAME_ARENA_1F_CTF ) {
			if( flagStatus[ i ].state == FS_TAKEN_MARINE ) {
				flagTeam = TEAM_MARINE;
			} else if( flagStatus[ i ].state == FS_TAKEN_STROGG ) {
				flagTeam = TEAM_STROGG;
			}
		}

		if( flagStatus[ i ].state == FS_DROPPED && !noSounds ) {
			if ( !player->spectating ) {
				if( flagTeam == player->team ) {
					// our flag was dropped, so the enemy dropped it
					gameLocal.mpGame.ScheduleAnnouncerSound ( AS_CTF_ENEMY_DROPS_FLAG, gameLocal.time, -1, true );
				} else {
					gameLocal.mpGame.ScheduleAnnouncerSound ( AS_CTF_YOUR_TEAM_DROPS_FLAG, gameLocal.time, -1, true );
				}
			}

		if( player->mphud ) {
				player->mphud->SetStateInt( "team", flagTeam );
				player->mphud->HandleNamedEvent( "flagDrop" );

				if ( !player->spectating ) {
					if ( flagTeam == player->team ) {
						player->mphud->SetStateString( "main_notice_text", common->GetLocalizedString( "#str_107723" ) );
					} else {
						player->mphud->SetStateString( "main_notice_text", common->GetLocalizedString( "#str_104420" ) );
					}

					player->mphud->HandleNamedEvent( "main_notice" );
				}
			}
		} else if( flagStatus[ i ].state == FS_AT_BASE ) {
			if( ((rvCTFGameState*)previousGameState)->flagStatus[ i ].state == FS_TAKEN && !noSounds ) {
				// team scores
				if ( !player->spectating ) {
					if( flagTeam == player->team ) {
						if( gameLocal.mpGame.CanCapture( gameLocal.mpGame.OpposingTeam( flagTeam ) ) ) {
							gameLocal.mpGame.ScheduleAnnouncerSound ( AS_TEAM_ENEMY_SCORES, gameLocal.time );
						}
					} else {
						if( gameLocal.mpGame.CanCapture( gameLocal.mpGame.OpposingTeam( flagTeam ) ) ) {
							gameLocal.mpGame.ScheduleAnnouncerSound ( AS_TEAM_YOU_SCORE, gameLocal.time );
						}
					}
				}
			} else if( ((rvCTFGameState*)previousGameState)->flagStatus[ i ].state == FS_DROPPED && !noSounds ) {
				// flag returned
				if ( !player->spectating ) {
					if( flagTeam == player->team ) {
						gameLocal.mpGame.ScheduleAnnouncerSound ( AS_CTF_YOUR_FLAG_RETURNED, gameLocal.time, -1, true );
					} else {
						gameLocal.mpGame.ScheduleAnnouncerSound ( AS_CTF_ENEMY_RETURNS_FLAG, gameLocal.time, -1, true );
					}
				}
			}
			
			if( player->mphud ) {
				player->mphud->SetStateInt( "team", flagTeam );
				player->mphud->HandleNamedEvent( "flagReturn" );
			}
		} else if( flagStatus[ i ].state == FS_TAKEN || flagStatus[ i ].state == FS_TAKEN_STROGG || flagStatus[ i ].state == FS_TAKEN_MARINE ) {
			// flag taken
			if( flagTeam == player->team ) {
				if ( !player->spectating ) {
					if ( !noSounds ) {
						gameLocal.mpGame.ScheduleAnnouncerSound ( AS_CTF_ENEMY_HAS_FLAG, gameLocal.time, -1, true );
					}
				}

				if ( !player->spectating ) {
					if ( player->mphud ) {
						player->mphud->SetStateString( "main_notice_text", common->GetLocalizedString( "#str_107722" ) );
						player->mphud->HandleNamedEvent( "main_notice" );
					}
				}
			} else {
				if ( flagStatus[ i ].clientNum == gameLocal.localClientNum ) {
					if ( !noSounds ) {
						gameLocal.mpGame.ScheduleAnnouncerSound ( AS_CTF_YOU_HAVE_FLAG, gameLocal.time, -1, true );
					}					

					if ( !player->spectating ) {
						// shouchard:  inform the GUI that you've taken the flag
						player->mphud->SetStateString( "main_notice_text", common->GetLocalizedString( "#str_104419" ) );
						player->mphud->HandleNamedEvent( "main_notice" );
					}
				} else if ( !noSounds ) {
					if ( !player->spectating ) {
						gameLocal.mpGame.ScheduleAnnouncerSound ( AS_CTF_YOUR_TEAM_HAS_FLAG, gameLocal.time, -1, true );
					}
				}
			}

			if( player->mphud ) {
				player->mphud->SetStateInt( "team", flagTeam );
				player->mphud->HandleNamedEvent ( "flagTaken" );
			}
		}
	}
}

/*
================
rvCTFGameState::Run
================
*/
void rvCTFGameState::Run( void ) {
	// run common stuff	
	rvGameState::Run();

	switch( currentState ) {
		case GAMEON: {
			int team = ( ( gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) >= gameLocal.serverInfo.GetInt( "si_captureLimit" ) ) ? TEAM_MARINE : ( ( gameLocal.mpGame.GetScoreForTeam( TEAM_STROGG ) >= gameLocal.serverInfo.GetInt( "si_captureLimit" ) ) ? TEAM_STROGG : -1 ) );
			if( gameLocal.serverInfo.GetInt( "si_captureLimit" ) <= 0 ) {
				// no capture limit games
				team = -1;
			}
			bool tiedForFirst = gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) == gameLocal.mpGame.GetScoreForTeam( TEAM_STROGG );
			if ( team >= 0 && !tiedForFirst ) {
				if ( !fragLimitTimeout ) {
					common->DPrintf( "enter capture limit timeout, team %d is leader\n", team );
					fragLimitTimeout = gameLocal.time + CAPTURELIMIT_DELAY;
				}
				if ( gameLocal.time > fragLimitTimeout ) {
					NewState( GAMEREVIEW );

					gameLocal.mpGame.PrintMessageEvent( -1, MSG_CAPTURELIMIT, team );

				}
			} else if ( fragLimitTimeout ) {
				// frag limit was hit and cancelled. means the two teams got even during FRAGLIMIT_DELAY
				// enter sudden death, the next frag leader will win
				// OR the winner lost a point in the frag delay, and there's no tie, so no one wins, game on.
				if( tiedForFirst && ( gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) >= gameLocal.serverInfo.GetInt( "si_captureLimit" ) ))	{
					NewState( SUDDENDEATH );
				}
				fragLimitTimeout = 0; 
			} else if ( gameLocal.mpGame.TimeLimitHit() ) {
				gameLocal.mpGame.PrintMessageEvent( -1, MSG_TIMELIMIT );
				if( tiedForFirst ) {
					// if tied at timelimit hit, goto sudden death
					fragLimitTimeout = 0;
					NewState( SUDDENDEATH );
				} else {
					// or just end the game
					NewState( GAMEREVIEW );
				}
			} else if( tiedForFirst && team >= 0 ) {
				// check for the rare case that two teams both hit the fraglimit the same frame
				// two people tied at fraglimit, advance to sudden death after a delay
				fragLimitTimeout = gameLocal.time + CAPTURELIMIT_DELAY;
			}
			break;
		}

		case SUDDENDEATH: {
			int team = gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) > gameLocal.mpGame.GetScoreForTeam( TEAM_STROGG ) ? TEAM_MARINE : TEAM_STROGG;
			bool tiedForFirst = false;
			if( gameLocal.mpGame.GetScoreForTeam( TEAM_MARINE ) == gameLocal.mpGame.GetScoreForTeam( TEAM_STROGG ) ) {
				team = -1;
				tiedForFirst = true;
			}

			if ( team >= 0 && !tiedForFirst ) {
				if ( !fragLimitTimeout ) {
					common->DPrintf( "enter sudden death FragLeader timeout, team %d is leader\n", team );
					fragLimitTimeout = gameLocal.time + CAPTURELIMIT_DELAY;
				}
				if ( gameLocal.time > fragLimitTimeout ) {
					NewState( GAMEREVIEW );
					gameLocal.mpGame.PrintMessageEvent( -1, MSG_CAPTURELIMIT, team );
				}
			} else if ( fragLimitTimeout ) {
				gameLocal.mpGame.PrintMessageEvent( -1, MSG_HOLYSHIT );
				fragLimitTimeout = 0;
			} 

			break;
		}
	}
}

/*
================
rvCTFGameState::SetFlagState
================
*/
void rvCTFGameState::SetFlagState( int flag, flagState_t newState ) {
	if ( !gameLocal.isServer ) {
		return;
	}

	assert( gameLocal.isServer && ( flag >= 0 && flag < TEAM_MAX ) && IsType( rvCTFGameState::GetClassType() ) );

	flagStatus[ flag ].state = newState;
}

/*
================
rvCTFGameState::SetFlagCarrier
================
*/
void rvCTFGameState::SetFlagCarrier( int flag, int clientNum ) {
	assert( gameLocal.isServer && ( flag >= 0 && flag < TEAM_MAX ) && (clientNum >= 0 && clientNum < MAX_CLIENTS) && IsType( rvCTFGameState::GetClassType() ) );

	flagStatus[ flag ].clientNum = clientNum;
}

/*
================
rvCTFGameState::operator==
================
*/
bool rvCTFGameState::operator==( const rvCTFGameState& rhs ) const {
	if( (rvGameState&)(*this) != (rvGameState&)rhs ) {
		return false;
	}

	for( int i = 0; i < TEAM_MAX; i++ ) {
		if( flagStatus[ i ] != rhs.flagStatus[ i ] ) {
			return false;
		}
	}

	for( int i = 0; i < MAX_AP; i++ ) {
		if( apState[ i ] != rhs.apState[ i ] ) {
			return false;
		}
	}

	return true;
}

/*
================
rvCTFGameState::operator=
================
*/
rvCTFGameState& rvCTFGameState::operator=( const rvCTFGameState& rhs ) {
	(rvGameState&)(*this) = (rvGameState&)rhs;

	for( int i = 0; i < TEAM_MAX; i++ ) {
		flagStatus[ i ] = rhs.flagStatus[ i ];
	}

	for( int i = 0; i < MAX_AP; i++ ) {
		apState[ i ] = rhs.apState[ i ];
	}

 	return (*this);
}

/*
===============================================================================

rvTourneyGameState

Game state info for tourney

===============================================================================
*/

/*
================
rvTourneyGameState::rvTourneyGameState
================
*/
rvTourneyGameState::rvTourneyGameState( bool allocPrevious ) : rvGameState( false ) {
	Clear();

	if( allocPrevious ) {
		previousGameState = new rvTourneyGameState( false );
	} else {
		previousGameState = NULL;
	}

	packTourneyHistory = false;

	trackPrevious = allocPrevious;
}

/*
================
rvTourneyGameState::Clear
================
*/
void rvTourneyGameState::Clear( void ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	// mekberg: clear previous game state.
	if ( previousGameState ) {
		previousGameState->Clear();
	}		

	rvGameState::Clear();

	tourneyState = TS_INVALID;

	for( int i = 0; i < MAX_ARENAS; i++ ) {
		arenas[ i ].Clear( false );
		arenas[ i ].SetArenaID( i );
	}

	for( int i = 0; i < MAX_ROUNDS; i++ ) {
		for( int j = 0; j < MAX_ARENAS; j++ ) {
			tourneyHistory[ i ][ j ].playerOne[ 0 ] = '\0';
			tourneyHistory[ i ][ j ].playerTwo[ 0 ] = '\0';
			tourneyHistory[ i ][ j ].playerOneNum = -1;
			tourneyHistory[ i ][ j ].playerTwoNum = -1;
			tourneyHistory[ i ][ j ].playerOneScore = -1;
			tourneyHistory[ i ][ j ].playerTwoScore = -1;
		}
	}

	startingRound = 0;
	maxRound = 0;
	round = -1;
	byeArena = -1;
	tourneyCount = 0;
	roundTimeout = 0;
}

/*
================
rvTourneyGameState::Reset
================
*/
void rvTourneyGameState::Reset( void ) {
	for( int i = 0; i < MAX_ARENAS; i++ ) {
		arenas[ i ].Clear( false );
		arenas[ i ].SetArenaID( i );
	}

	for( int i = 0; i < MAX_ROUNDS; i++ ) {
		for( int j = 0; j < MAX_ARENAS; j++ ) {
			tourneyHistory[ i ][ j ].playerOne[ 0 ] = '\0';
			tourneyHistory[ i ][ j ].playerTwo[ 0 ] = '\0';
			tourneyHistory[ i ][ j ].playerOneNum = -1;
			tourneyHistory[ i ][ j ].playerTwoNum = -1;
			tourneyHistory[ i ][ j ].playerOneScore = -1;
			tourneyHistory[ i ][ j ].playerTwoScore = -1;
		}
	}

	startingRound = 0;
	maxRound = 0;
	round = -1;
	byeArena = -1;
	roundTimeout = 0;
}

/*
================
rvTourneyGameState::Run
================
*/
void rvTourneyGameState::Run( void ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	rvGameState::Run();

	switch( currentState ) {
		case GAMEON: {
			// check to see if we need to advance to the next round
			if( nextState == GAMEREVIEW ) {
				return;
			}

			bool roundDone = true;

			for( int i = 0; i < MAX_ARENAS; i++ ) {
				if( arenas[ i ].GetState() == AS_INACTIVE ) {
					continue;
				}

				arenas[ i ].UpdateState();

				if( arenas[ i ].GetState() != AS_DONE ) {
					// check to see if we're done
					roundDone = false;
				}
			}
			if( roundDone && !roundTimeout ) {
				roundTimeout = gameLocal.time + FRAGLIMIT_DELAY;
			}

			if( roundDone && gameLocal.time > roundTimeout ) {
				int pastRound = round - 1; //rounds are 1 indexed
				idList<rvPair<idPlayer*, int> > advancingPlayers;

				round++;
				roundTimeout = 0;

				assert( round >= 2 );

				// copy over tourney history for the previous round
				UpdateTourneyHistory( pastRound );			

				// transmit history
				forceTourneyHistory = true;

				// add who will advance to the next round
				for( int i = 0; i < MAX_ARENAS; i++ ) {
					if( arenas[ i ].GetState() == AS_DONE && arenas[ i ].GetWinner() ) {
						advancingPlayers.Append( rvPair<idPlayer*, int>( arenas[ i ].GetWinner(), i ) );
						gameLocal.mpGame.AddPlayerWin( arenas[ i ].GetWinner(), 1 );
					} 
				}

				// when only one player advances, that player has won the tournament
				if( advancingPlayers.Num() == 1 ) {
					idPlayer* winner = advancingPlayers[ 0 ].First();
					assert( winner );

					for( int i = 0; i < MAX_ARENAS; i++ ) {
						arenas[ i ].Clear();
					}

					gameLocal.Printf( "Round %d: %d (%s) has won the tourney!\n", round - 1, winner->entityNumber, gameLocal.userInfo[ winner->entityNumber ].GetString( "ui_name" ) );
					// award 5 wins for winning the entire tournament
					gameLocal.mpGame.AddPlayerWin( winner, 5 );
					//asalmon: For Xbox 360 do not delay game review
#ifdef _XENON
					NewState( GAMEREVIEW );
#else
					nextState = GAMEREVIEW;
					nextStateTime = gameLocal.time + 5000;
#endif
					return;
				}

				// see if we have to swap a player from the byeArena this round
				bool swapBye = false;
				int thisByeArena = -1;

				if( byeArena >= 0 ) {
					// look at the bye arena from the round that just ended
					thisByeArena = byeArena / (round - 1);

					// if the bye arena is playing in the next round, there's no need to switch players
					if( !(thisByeArena % 2) ) {
						if( arenas[ thisByeArena ].GetState() == AS_DONE && arenas[ thisByeArena + 1 ].GetState() == AS_DONE ) {
							assert( arenas[ thisByeArena ].GetWinner() && arenas[ thisByeArena + 1 ].GetWinner() );
							swapBye = false;
						} else {
							swapBye = true;
						}
					} else {
 						if( arenas[ thisByeArena ].GetState() == AS_DONE && arenas[ thisByeArena - 1 ].GetState() == AS_DONE ) {
							assert( arenas[ thisByeArena ].GetWinner() && arenas[ thisByeArena - 1 ].GetWinner() );
							swapBye = false;
						} else {
							swapBye = true;
						}
					}
				}

				idPlayer* oldByePlayer = NULL;
				idPlayer* newByePlayer = NULL;				

				if( swapBye ) {
					oldByePlayer = arenas[ thisByeArena ].GetWinner();

					// pick a new bye player only from players who will be fighting next round
					// i.e., don't swap the bye player with a player who will have a disconnect bye this upcoming round
					idList<rvPair<idPlayer*, int> > nextRoundPlayers;
					for( int i = 0; i < MAX_ARENAS; i += 2 ) {
						if( arenas[ i ].GetState() == AS_DONE && arenas[ i ].GetWinner() && arenas[ i + 1 ].GetState() == AS_DONE && arenas[ i + 1 ].GetWinner() ) {
							nextRoundPlayers.Append( rvPair<idPlayer*, int>( arenas[ i ].GetWinner(), i ) );
							nextRoundPlayers.Append( rvPair<idPlayer*, int>( arenas[ i + 1 ].GetWinner(), i + 1 ) );
						}
					}
						
					if( nextRoundPlayers.Num() ) {
						newByePlayer = nextRoundPlayers[ gameLocal.random.RandomInt( nextRoundPlayers.Num() ) ].First();
					}
				} 

				// assign arenas for the next round
				for( int i = 0; i < MAX_ARENAS; i += 2 ) {
					idPlayer* advanceOne = arenas[ i ].GetWinner();
					idPlayer* advanceTwo = arenas[ i + 1 ].GetWinner();

					arenas[ i ].Clear();
					arenas[ i + 1 ].Clear();

					// assign new arenas, swapping oldByePlayer with newByePlayer
					if( newByePlayer && oldByePlayer ) {
						if( advanceOne == newByePlayer ) {
							advanceOne = oldByePlayer;
						} else if( advanceTwo == newByePlayer ) {
							advanceTwo = oldByePlayer;
						} else if( advanceOne == oldByePlayer ) {
							advanceOne = newByePlayer;
						} else if( advanceTwo == oldByePlayer ) {
							advanceTwo = newByePlayer;
						}
					} 
					
					if( advanceOne || advanceTwo ) {
						arenas[ (i / 2) ].AddPlayers( advanceOne, advanceTwo );

						if( advanceOne ) {
							advanceOne->JoinInstance( (i / 2) );
							advanceOne->ServerSpectate( false );
						}

						if( advanceTwo ) {
							advanceTwo->JoinInstance( (i / 2) );
							advanceTwo->ServerSpectate( false );
						}

						gameLocal.Printf( "Round %d: Arena %d is starting play with players %d (%s) and %d (%s)\n", round, (i / 2), advanceOne ? advanceOne->entityNumber : -1, advanceOne ? advanceOne->GetUserInfo()->GetString( "ui_name" ) : "NULL", advanceTwo ? advanceTwo->entityNumber : -1, advanceTwo ? advanceTwo->GetUserInfo()->GetString( "ui_name" ) : "NULL" );

						arenas[ (i / 2) ].Ready();
					}
				}

				// if we have a bye player, figure out who they are going to play
				/*
				// this is an index into the arena the bye player will fight in
				int previousByeOpponent = -1;

				if( byePlayer ) {
					lowestScoreDeltas.Sort( rvPair<int, int>::rvPairSecondCompare );

					// prune down scoreDeltas to the bottom X players
					for( i = 0; i < lowestScoreDeltas.Num() - 1; i++ ) {
						if( lowestScoreDeltas[ i ].Second() != lowestScoreDeltas[ lowestScoreDeltas.Num() - 1 ].Second() ) {
							lowestScoreDeltas.RemoveIndex( i );
							i--;
						}
					}
					
					assert( lowestScoreDeltas.Num() );

					// pick a bye player
					previousByeOpponent = lowestScoreDeltas[ gameLocal.random.RandomInt( lowestScoreDeltas.Num() ) ].First();
				}

				bool arenaProcessed[ MAX_ARENAS ];
				memset( arenaProcessed, 0, sizeof( arenaProcessed ) );

				// setup the arenas for the next round each arena winner plays the winner from the next arena
				for( i = 0; i < MAX_ARENAS; i++ ) {
					if( arenas[ i ].GetState() != AS_DONE || arenaProcessed[ i ] ) {
						continue;
					}

					idPlayer* advanceOne = arenas[ i ].GetWinner();
					idPlayer* advanceTwo = NULL;

					assert( advanceOne );

					// remove this player from the arena, so when the arena gets cleared we don't get forced to spectate
					arenas[ i ].RemovePlayer( advanceOne );
					
					// copy over the results of this tourney to the tourney history
					if( arenas[ i ].GetPlayers()[ 0 ] && arenas[ i ].GetPlayers()[ 1 ] ) {
						UpdateTourneyHistory( round - 2, i, arenas[ i ].GetPlayers()[ 0 ], arenas[ i ].GetPlayers()[ 1 ] );
					}
					
					arenaProcessed[ i ] = true;

					// find an opponent for the next round
					if( i == newByeIndex ) {
						// we are the new bye player, will be set after this loop
						continue;
					} else if( i == previousByeOpponent ) {
						// we are playing the previous round's bye player
						assert( byePlayer );
						advanceTwo = byePlayer;
						byePlayer = NULL;
					} else {
						// find the opponent in the next arena
						int j;
						for( j = i + 1; j < MAX_ARENAS; j++ ) {
							// don't match this player with the player becoming a bye player or the player slated to play
							// the previous round's bye player
							if( arenas[ j ].GetState() != AS_DONE || j == newByeIndex || j == previousByeOpponent ) {
								continue;
							}

							advanceTwo = arenas[ j ].GetWinner();
							arenaProcessed[ j ] = true;

							arenas[ j ].RemovePlayer( advanceTwo );

							if( arenas[ j ].GetPlayers()[ 0 ] && arenas[ j ].GetPlayers()[ 1 ] ) {
								UpdateTourneyHistory( round - 2, j, arenas[ j ].GetPlayers()[ 0 ], arenas[ j ].GetPlayers()[ 1 ] );							
							}
							
							arenas[ j ].Clear();

							break;
						}

						assert( j < MAX_ARENAS );
					}

					assert( advanceTwo );

					// place advanceOne and advanceTwo in the next arena
					int arena = FirstAvailableArena();

					arenas[ arena ].Clear();
					arenas[ arena ].AddPlayers( advanceOne, advanceTwo );

					// award bracket advancement
					gameLocal.mpGame.AddPlayerWin( advanceOne, 1 );
					gameLocal.mpGame.AddPlayerWin( advanceTwo, 1 );

					advanceOne->ServerSpectate( false );
					advanceTwo->ServerSpectate( false );

					gameLocal.Printf( "Round %d: Arena %d is starting play with players %d (%s) and %d (%s)\n", round, arena, advanceOne->entityNumber, advanceOne->GetUserInfo()->GetString( "ui_name" ), advanceTwo->entityNumber, advanceTwo->GetUserInfo()->GetString( "ui_name" ) );

					advanceOne->JoinInstance( arena );
					advanceTwo->JoinInstance( arena );

					arenas[ arena ].Ready();
				}

				// at this point, we either didn't have a byePlayer, or the previous byePlayer has been
				// placed in an arena for this round
				assert( !byePlayer );

				if( newByeClientNum >= 0 ) {
					assert( newByeClientNum >= 0 && newByeClientNum < MAX_CLIENTS ); 
					byePlayer = (idPlayer*)gameLocal.entities[ newByeClientNum ];

					// place the byePlayer in a valid instance
					byePlayer->JoinInstance( 0 );
				} else {
					assert( !setupBye );
				}			

				// clear any remaining arenas that weren't re-used
				for( i = 0; i < MAX_ARENAS; i++ ) {
					if( arenas[ i ].GetState() != AS_WARMUP ) {
						arenas[ i ].Clear(); 
					}
				}*/

				// setup the next round - even though sometimes we process 2 arenas at once, we have to run through all of them, incase
				// the setup changes mid-round because of people disconnecting
				/*for( i = 0; i < MAX_ARENAS; i++ ) {
					// only consider active arenas
					if( arenas[ i ].GetState() == AS_INACTIVE ) {
						continue;
					}

					int j = i + 1;

					idPlayer* advanceOne = arenas[ i ].GetWinner();
					idPlayer* advanceTwo = j < MAX_ARENAS ? arenas[ j ].GetWinner() : NULL;

					assert( advanceOne );
					assert( arenas[ i ].GetLoser() );
	
					assert( arenas[ i ].GetPlayers()[ 0 ] && arenas[ i ].GetPlayers()[ 1 ] );

					if( historyOffset >= 0 && historyOffset < MAX_ROUNDS ) {
						tourneyHistory[ historyOffset ][ i ].playerOneNum = arenas[ i ].GetPlayers()[ 0 ]->entityNumber;
						tourneyHistory[ historyOffset ][ i ].playerTwoNum = arenas[ i ].GetPlayers()[ 1 ]->entityNumber;
						tourneyHistory[ historyOffset ][ i ].playerOne[ MAX_TOURNEY_HISTORY_NAME_LEN - 1 ] = '\0';
						tourneyHistory[ historyOffset ][ i ].playerTwo[ MAX_TOURNEY_HISTORY_NAME_LEN - 1 ] = '\0';

						tourneyHistory[ historyOffset ][ i ].playerOneScore = arenas[ i ].GetPlayerScore( 0 );
						tourneyHistory[ historyOffset ][ i ].playerTwoScore = arenas[ i ].GetPlayerScore( 1 );
					}
					
					// if this arena is active & done, but the next one is inactive we need to check for a bye
					if( j >= MAX_ARENAS || arenas[ j ].GetState() == AS_INACTIVE ) {
						// if we're the last arena and there are no byes and we're done, the tourney is done
						if( i == 0 && byePlayer == NULL ) {
							gameLocal.Printf( "Round %d: %d (%s) has won the tourney!\n", round - 1, advanceOne->entityNumber, gameLocal.userInfo[ advanceOne->entityNumber ].GetString( "ui_name" ) );
							// award 5 wins for winning the entire tournament
							gameLocal.mpGame.AddPlayerWin( advanceOne, 5 );
//asalmon: For Xbox 360 do not delay game review
#ifdef _XENON
							NewState( GAMEREVIEW );
#else
							nextState = GAMEREVIEW;
							nextStateTime = gameLocal.time + 5000;
#endif
							break;
						}

						arenas[ i ].Clear();

						// if no player has a bye, we become the byePlayer
						if( byePlayer == NULL ) {
							byePlayer = advanceOne;
							continue;
						} else {
							// otherwise, we play the current byePlayer
							advanceTwo = byePlayer;
							byePlayer = NULL;
						}
					} else {
						// arena i and arena i + 1 can fight eachother

						// GetWinner() might return NULL if there's a tie, but if 
						// there is a tie round state should not get to AS_DONE
						assert( advanceTwo );

						assert( arenas[ j ].GetPlayers()[ 0 ] && arenas[ j ].GetPlayers()[ 1 ] );

						if( historyOffset >= 0 && historyOffset < MAX_ROUNDS ) {
							tourneyHistory[ historyOffset ][ j ].playerOneNum = arenas[ j ].GetPlayers()[ 0 ]->entityNumber;
							tourneyHistory[ historyOffset ][ j ].playerTwoNum = arenas[ j ].GetPlayers()[ 1 ]->entityNumber;

							tourneyHistory[ historyOffset ][ j ].playerOne[ MAX_TOURNEY_HISTORY_NAME_LEN - 1 ] = '\0';
							tourneyHistory[ historyOffset ][ j ].playerTwo[ MAX_TOURNEY_HISTORY_NAME_LEN - 1 ] = '\0';

							tourneyHistory[ historyOffset ][ j ].playerOneScore = arenas[ j ].GetPlayerScore( 0 );
							tourneyHistory[ historyOffset ][ j ].playerTwoScore = arenas[ j ].GetPlayerScore( 1 );
						}

						scoreDeltas.Append( rvPair<int, int>( j, gameLocal.mpGame.GetTeamScore( arenas[ j ].GetWinner() ) - gameLocal.mpGame.GetTeamScore( arenas[ j ].GetLoser() ) ) );

						// clear arenas
						arenas[ i ].Clear();
						arenas[ j ].Clear();
					}

					// winners of arenas X and X+1 compete in arena 
					// X / 2 in the next round.  This arena should have been
					// cleared in a previous loop iteration
					arenas[ (i / 2) ].AddPlayers( advanceOne, advanceTwo );
					
					// award bracket advancement
					gameLocal.mpGame.AddPlayerWin( advanceOne, 1 );
					gameLocal.mpGame.AddPlayerWin( advanceTwo, 1 );

					advanceOne->ServerSpectate( false );
					advanceTwo->ServerSpectate( false );

					gameLocal.Printf( "Round %d: Arena %d is starting play with players %d (%s) and %d (%s)\n", round, i / 2, advanceOne->entityNumber, advanceOne->GetUserInfo()->GetString( "ui_name" ), advanceTwo->entityNumber, advanceTwo->GetUserInfo()->GetString( "ui_name" ) );

					advanceOne->JoinInstance( (i / 2) );
					advanceTwo->JoinInstance( (i / 2) );

					arenas[ (i / 2) ].Ready();
				}*/

				// in certain situations, a player will get bye'd to the finals and needs to win
				/*if ( round >= maxRound && GetNumArenas() == 0 && byePlayer ) {
					gameLocal.Printf( "Round %d: %d (%s) has won the tourney!\n", round, byePlayer->entityNumber, gameLocal.userInfo[ byePlayer->entityNumber ].GetString( "ui_name" ) );
					// award 5 wins for winning the entire tournament
					gameLocal.mpGame.AddPlayerWin( byePlayer, 5 );
//asalmon: For Xbox 360 do not delay game review
#ifdef _XENON
							NewState( GAMEREVIEW );
#else
							nextState = GAMEREVIEW;
							nextStateTime = gameLocal.time + 5000;
#endif
					break;
				}*/

				// if the bye player is the same as before we setup the new round, swap the byeplayer
				// with someone else - so one player doesn't get a bye all the way to the finals.
			/*	if( byePlayer && byePlayer == formerByePlayer ) {
					assert( scoreDeltas.Num() ); // something is very wrong....

					// Pick t
					scoreDeltas.Sort( rvPair<int, int>::rvPairSecondCompare );
					int i;
					for( i = 1; i < scoreDeltas.Num(); i++ ) {
						if( scoreDeltas[ i ].Second() != scoreDeltas[ i - 1 ].Second() ) {
							break;
						} 
					}
					int arena = gameLocal.random.RandomInt( i ); // this is an index to the previous round's arenas
					rvTourneyArena& swapArena = arenas[ (scoreDeltas[ arena ].First() / 2) ];
					assert( swapArena.GetPlayers()[ 0 ] && swapArena.GetPlayers()[ 1 ] );
					idPlayer* t = byePlayer;
					if( arena & 2 ) {
						// the highest delta was the first players
						byePlayer = swapArena.GetPlayers()[ 0 ];
						swapArena.AddPlayers( t, swapArena.GetPlayers()[ 1 ] );
					} else {
						// the highest delta was the second player
						byePlayer = swapArena.GetPlayers()[ 1 ];
						swapArena.AddPlayers( swapArena.GetPlayers()[ 0 ], t );					
					}
					assert( swapArena.GetPlayers()[ 0 ] && swapArena.GetPlayers()[ 1 ] );

					swapArena.GetPlayers()[ 0 ]->JoinInstance( swapArena.GetArenaID() );
					swapArena.GetPlayers()[ 1 ]->JoinInstance( swapArena.GetArenaID() );
					swapArena.GetPlayers()[ 0 ]->ServerSpectate( false );
					swapArena.GetPlayers()[ 1 ]->ServerSpectate( false );
					byePlayer->ServerSpectate( true );
					swapArena.Ready();
					gameLocal.Printf( "Round %d: Arena %d is re-starting play (bye-swap) with players %d (%s) and %d (%s)\n", round, swapArena.GetArenaID(), swapArena.GetPlayers()[ 0 ]->entityNumber, swapArena.GetPlayers()[ 0 ]->GetUserInfo()->GetString( "ui_name" ), swapArena.GetPlayers()[ 1 ]->entityNumber, swapArena.GetPlayers()[ 1 ]->GetUserInfo()->GetString( "ui_name" ) );
				}	*/

				// once the new round is setup, go through and make sure all the spectators are in valid arena
				for( int i = 0; i < gameLocal.numClients; i++ ) {
					idPlayer* p = (idPlayer*)gameLocal.entities[ i ];

					// re-select our arena since round changed
					if( p && p->spectating ) {
						rvTourneyArena& thisArena = arenas[ p->GetArena() ];
						if( thisArena.GetState() != AS_WARMUP ) {
							p->JoinInstance( GetNextActiveArena( p->GetArena() ) );
						}
					}
				}
			}

			break;
		}
	}
}

/*
================
rvTourneyGameState::NewState
================
*/
void rvTourneyGameState::NewState( mpGameState_t newState ) {
	assert( (newState != currentState) && gameLocal.isServer );

	switch( newState ) {
		case WARMUP: {
			Reset();
			// force everyone to spectate
			for( int i = 0; i < MAX_CLIENTS; i++ ) {
				if( gameLocal.entities[ i ] ) {
					((idPlayer*)gameLocal.entities[ i ])->ServerSpectate( true );
				}
			}
			break;
		}
		case GAMEON: {
			tourneyCount++;
			SetupInitialBrackets();
			break;
		}
	}

	rvGameState::NewState( newState );
}

/*
================
rvTourneyGameState::GameStateChanged
================
*/
void rvTourneyGameState::GameStateChanged( void ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );
	rvGameState::GameStateChanged();
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();

	int oldRound = ((rvTourneyGameState*)previousGameState)->round;

	if( round != oldRound ) {
		if ( round > maxRound && gameLocal.GetLocalPlayer() ) {
			gameLocal.GetLocalPlayer()->ForceScoreboard( true, gameLocal.time + 5000 );
			gameLocal.mpGame.ScheduleAnnouncerSound( AS_TOURNEY_DONE, gameLocal.time );
		}

		// we're starting a new round
		gameLocal.mpGame.tourneyGUI.RoundStart();
			
		// play the sound a bit after round restart to let spawn sounds settle
		gameLocal.mpGame.ScheduleAnnouncerSound( (announcerSound_t)(AS_TOURNEY_PRELIMS + idMath::ClampInt( 1, maxRound, round - 1 ) ), gameLocal.time + 1500);
	}

	for( int i = 0; i < MAX_ARENAS; i++ ) {
		rvTourneyArena& thisArena = arenas[ i ];
		rvTourneyArena& previousArena = ((rvTourneyGameState*)previousGameState)->arenas[ i ];
		
		if( ((thisArena.GetPlayers()[ 0 ] != previousArena.GetPlayers()[ 0 ]) ||
			(thisArena.GetPlayers()[ 1 ] != previousArena.GetPlayers()[ 1 ]) ||
			(round != ((rvTourneyGameState*)previousGameState)->round )) /*&& 
			!((thisArena.GetPlayers()[ 0 ] == NULL && thisArena.GetPlayers()[ 1 ] != NULL) ||
			  (thisArena.GetPlayers()[ 0 ] != NULL && thisArena.GetPlayers()[ 1 ] == NULL) ) */) {

			// don't re-init arenas that have changed and been done for a while
			if( thisArena.GetState() != AS_DONE || previousArena.GetState() != AS_DONE ) {
				gameLocal.mpGame.tourneyGUI.ArenaStart( i );
			}

			if( localPlayer && thisArena.GetPlayers()[ 0 ] == localPlayer ) {
				gameLocal.mpGame.tourneyGUI.ArenaSelect( i, TGH_PLAYER_ONE );
			}
			if( localPlayer && thisArena.GetPlayers()[ 1 ] == localPlayer ) {
				gameLocal.mpGame.tourneyGUI.ArenaSelect( i, TGH_PLAYER_TWO );
			}
		}

		if( localPlayer->GetArena() == thisArena.GetArenaID() && thisArena.GetState() == AS_SUDDEN_DEATH && thisArena.GetState() != previousArena.GetState() ) {
			gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_SUDDEN_DEATH, gameLocal.time, thisArena.GetArenaID() );
			gameLocal.GetLocalPlayer()->GUIMainNotice( common->GetLocalizedString( "#str_104287" ) );
		}

		if( thisArena.GetState() == AS_DONE && thisArena.GetState() != previousArena.GetState() ) {
			if( thisArena.GetWinner() != previousArena.GetWinner() && thisArena.GetWinner() == gameLocal.GetLocalPlayer() ) {
				if( round >= maxRound ) {
					gameLocal.mpGame.ScheduleAnnouncerSound( AS_TOURNEY_WON, gameLocal.time );
				} else {
 					gameLocal.mpGame.ScheduleAnnouncerSound( AS_TOURNEY_ADVANCE, gameLocal.time );
				}
			} else if( thisArena.GetLoser() != previousArena.GetLoser() && thisArena.GetLoser() == gameLocal.GetLocalPlayer() ) {
				gameLocal.mpGame.ScheduleAnnouncerSound( AS_TOURNEY_ELIMINATED, gameLocal.time );
			}

			if( previousArena.GetWinner() == NULL && thisArena.GetWinner() != NULL ) {
				gameLocal.mpGame.tourneyGUI.ArenaDone( i );
			}

			// on the client, add this result to our tourneyHistory
			//if( gameLocal.isClient && thisArena.GetPlayers()[ 0 ] && thisArena.GetPlayers()[ 1 ] && (round - 1 ) >= 0 && (round - 1) < MAX_ROUNDS ) {
			//	tourneyHistory[ round - 1 ][ i ].playerOneScore = gameLocal.mpGame.GetTeamScore( thisArena.GetPlayers()[ 0 ] );
			//	tourneyHistory[ round - 1 ][ i ].playerTwoScore = gameLocal.mpGame.GetTeamScore( thisArena.GetPlayers()[ 1 ] );
			//}
		}

		if( localPlayer && (thisArena.GetState() == AS_WARMUP) && (thisArena.GetState() != previousArena.GetState()) && localPlayer->GetArena() == thisArena.GetArenaID() ) {
			gameLocal.mpGame.RemoveAnnouncerSound( AS_GENERAL_PREPARE_TO_FIGHT );
			gameLocal.mpGame.RemoveAnnouncerSound( AS_GENERAL_THREE );
			gameLocal.mpGame.RemoveAnnouncerSound( AS_GENERAL_TWO );
			gameLocal.mpGame.RemoveAnnouncerSound( AS_GENERAL_ONE );
			
			int warmupEndTime = gameLocal.time + ( gameLocal.serverInfo.GetInt( "si_countdown" ) * 1000 ) + 5000; 
			gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_PREPARE_TO_FIGHT, gameLocal.time + 5000 );
			gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_THREE, warmupEndTime - 3000 );
			gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_TWO, warmupEndTime - 2000 );
			gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_ONE, warmupEndTime - 1000 );

			localPlayer->vsMsgState = true;
			localPlayer->GUIMainNotice( va( "%s^0 vs. %s^0", thisArena.GetPlayerName( 0 ), thisArena.GetPlayerName( 1 ) ), true );
		}

		if( (thisArena.GetState() == AS_ROUND) && (thisArena.GetState() != previousArena.GetState()) ) {
  			if( localPlayer && localPlayer->GetArena() == thisArena.GetArenaID() ) {
  				gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_FIGHT, gameLocal.time );
				gameLocal.mpGame.ScheduleTimeAnnouncements();
  			}
		}
	}

	if( ((rvTourneyGameState*)previousGameState)->currentState != currentState ) {
		if( currentState == WARMUP ) {
			gameLocal.mpGame.tourneyGUI.PreTourney();
		} else if( currentState == COUNTDOWN ) {	
			if( currentState == COUNTDOWN && ((rvTourneyGameState*)previousGameState)->currentState != INACTIVE ) {
				gameLocal.mpGame.ScheduleAnnouncerSound( AS_TOURNEY_START, gameLocal.time);
			}
		} else if( currentState == GAMEON ) {
			gameLocal.mpGame.tourneyGUI.TourneyStart();
		}
	}

	if( packTourneyHistory ) {
		// apply history
		gameLocal.mpGame.tourneyGUI.SetupTourneyHistory( startingRound, round - 1, tourneyHistory );
		packTourneyHistory = false;
	}

	if( localPlayer && localPlayer->spectating ) {

		rvTourneyArena& thisArena = arenas[ localPlayer->GetArena() ];
		if( thisArena.GetPlayers()[ 0 ] == NULL || thisArena.GetPlayers()[ 1 ] == NULL || (localPlayer->spectating && localPlayer->spectator != thisArena.GetPlayers()[ 0 ]->entityNumber && localPlayer->spectator != thisArena.GetPlayers()[ 1 ]->entityNumber) ) {
			gameLocal.mpGame.tourneyGUI.ArenaSelect( localPlayer->GetArena(), TGH_BRACKET );
		} else if ( thisArena.GetPlayers()[ 0 ] == localPlayer || (localPlayer->spectating && thisArena.GetPlayers()[ 0 ]->entityNumber == localPlayer->spectator) ) {
			gameLocal.mpGame.tourneyGUI.ArenaSelect( localPlayer->GetArena(), TGH_PLAYER_ONE );
		} else if ( thisArena.GetPlayers()[ 1 ] == localPlayer || (localPlayer->spectating && thisArena.GetPlayers()[ 1 ]->entityNumber == localPlayer->spectator) ) {
			gameLocal.mpGame.tourneyGUI.ArenaSelect( localPlayer->GetArena(), TGH_PLAYER_TWO );
		}
	}

}

/*
================
msgTourney_t
Identifiers for transmitted state
================
*/
typedef enum {
	MSG_TOURNEY_TOURNEYSTATE,
	MSG_TOURNEY_STARTINGROUND,
	MSG_TOURNEY_ROUND,
	MSG_TOURNEY_MAXROUND,
	MSG_TOURNEY_HISTORY,
	MSG_TOURNEY_TOURNEYCOUNT,
	MSG_TOURNEY_ARENAINFO
} msgTourney_t;

/*
================
rvTourneyGameState::PackState
================
*/
void rvTourneyGameState::PackState( idBitMsg& outMsg ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );
	
	if( tourneyState != ((rvTourneyGameState*)previousGameState)->tourneyState ) {
		outMsg.WriteByte( MSG_TOURNEY_TOURNEYSTATE );
		outMsg.WriteByte( tourneyState );
	}

	if( startingRound != ((rvTourneyGameState*)previousGameState)->startingRound ) {
		outMsg.WriteByte( MSG_TOURNEY_STARTINGROUND );
		outMsg.WriteByte( startingRound );
	}

	if( round != ((rvTourneyGameState*)previousGameState)->round ) {
		outMsg.WriteByte( MSG_TOURNEY_ROUND );
		outMsg.WriteByte( round );
	}

	if( maxRound != ((rvTourneyGameState*)previousGameState)->maxRound ) {
		outMsg.WriteByte( MSG_TOURNEY_MAXROUND );
		outMsg.WriteByte( maxRound );
	}
	
	for( int i = 0; i < MAX_ARENAS; i++ ) {
		if( arenas[ i ] != ((rvTourneyGameState*)previousGameState)->arenas[ i ] ) {
			outMsg.WriteByte( MSG_TOURNEY_ARENAINFO + i );
			arenas[ i ].PackState( outMsg );
		}
	}

	if( packTourneyHistory || forceTourneyHistory ) {
		if( forceTourneyHistory ) {
			common->Warning("forcing down tourney history\n");
		}
		// don't write out uninitialized tourney history
		if( startingRound > 0 && round > 1 ) {
			outMsg.WriteByte( MSG_TOURNEY_HISTORY );

			// client might not yet have startingRound or round
			outMsg.WriteByte( startingRound ); 
			outMsg.WriteByte( round ); 
			outMsg.WriteByte( maxRound );

			for( int i = startingRound - 1; i <= Min( (round - 1), (maxRound - 1) ); i++ ) {
				for( int j = 0; j < MAX_ARENAS / (i + 1); j++ ) {
					outMsg.WriteString( tourneyHistory[ i ][ j ].playerOne, MAX_TOURNEY_HISTORY_NAME_LEN );
					outMsg.WriteChar( idMath::ClampChar( tourneyHistory[ i ][ j ].playerOneScore ) );
					outMsg.WriteString( tourneyHistory[ i ][ j ].playerTwo, MAX_TOURNEY_HISTORY_NAME_LEN );
					outMsg.WriteChar( idMath::ClampChar( tourneyHistory[ i ][ j ].playerTwoScore ) );
					outMsg.WriteByte( tourneyHistory[ i ][ j ].playerOneNum );
					outMsg.WriteByte( tourneyHistory[ i ][ j ].playerTwoNum );
				}
			}
			packTourneyHistory = false;
		}
	}

	if( tourneyCount != ((rvTourneyGameState*)previousGameState)->tourneyCount ) {
		outMsg.WriteByte( MSG_TOURNEY_TOURNEYCOUNT );
		outMsg.WriteByte( tourneyCount );
	}
}

/*
================
rvTourneyGameState::UnpackState
================
*/
void rvTourneyGameState::UnpackState( const idBitMsg& inMsg ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	while( inMsg.GetRemainingData() ) {
		int index = inMsg.ReadByte();
		
		switch( index ) {
			case MSG_TOURNEY_TOURNEYSTATE: {
				tourneyState = (tourneyState_t)inMsg.ReadByte();
				break;
			}
			case MSG_TOURNEY_STARTINGROUND: {
				startingRound = inMsg.ReadByte();
				break;
			}
			case MSG_TOURNEY_ROUND: {
				round = inMsg.ReadByte();
				// not a great way of doing this, should clamp the values
				if( round == 255 ) {
					round = -1;
				}
				break;
			}
			case MSG_TOURNEY_MAXROUND: {
				maxRound = inMsg.ReadByte();
				if( maxRound == 255 ) {
					maxRound = -1;
				}
				break;
			}
			case MSG_TOURNEY_HISTORY: {
				int startRound = inMsg.ReadByte();
				int rnd = inMsg.ReadByte();
				int maxr = inMsg.ReadByte();
				
				assert( rnd >= 1 ); // something is uninitialized

				for( int i = startRound - 1; i <= Min( (rnd - 1), (maxr - 1) ); i++ ) {
					for( int j = 0; j < MAX_ARENAS / (i + 1); j++ ) {
						inMsg.ReadString( tourneyHistory[ i ][ j ].playerOne, MAX_TOURNEY_HISTORY_NAME_LEN );
						tourneyHistory[ i ][ j ].playerOneScore = inMsg.ReadChar();
						inMsg.ReadString( tourneyHistory[ i ][ j ].playerTwo, MAX_TOURNEY_HISTORY_NAME_LEN );
						tourneyHistory[ i ][ j ].playerTwoScore = inMsg.ReadChar();
						tourneyHistory[ i ][ j ].playerOneNum = inMsg.ReadByte();
						tourneyHistory[ i ][ j ].playerTwoNum = inMsg.ReadByte();

						if( tourneyHistory[ i ][ j ].playerOneNum < 0 || tourneyHistory[ i ][ j ].playerOneNum >= MAX_CLIENTS ) {
							tourneyHistory[ i ][ j ].playerOneNum = -1;
						}

						if( tourneyHistory[ i ][ j ].playerTwoNum < 0 || tourneyHistory[ i ][ j ].playerTwoNum >= MAX_CLIENTS ) {
							tourneyHistory[ i ][ j ].playerTwoNum = -1;
						}
					}
				}

				// client side (and listen server) no reason to check for history change all the time
				packTourneyHistory = true;
				break;
			}
			case MSG_TOURNEY_TOURNEYCOUNT: {
				tourneyCount = inMsg.ReadByte();
				break;
			}
			default: {
				if( index >= MSG_TOURNEY_ARENAINFO && index < MSG_TOURNEY_ARENAINFO + MAX_ARENAS ) {
					arenas[ index - MSG_TOURNEY_ARENAINFO ].UnpackState( inMsg );
				} else {
					gameLocal.Error( "rvTourneyGameState::UnpackState() - Unknown data identifier '%d'\n", index );
				}
				break;
			}
		}

	}
}

/*
================
rvTourneyGameState::SendState
================
*/
void rvTourneyGameState::SendState( int clientNum ) {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	assert( gameLocal.isServer && trackPrevious && IsType( rvTourneyGameState::GetClassType() ) );

	if ( clientNum == -1 && (rvTourneyGameState&)(*this) == (rvTourneyGameState&)(*previousGameState) ) {
		return;
	}

	outMsg.Init( msgBuf, sizeof( msgBuf ) );
	outMsg.WriteByte( GAME_RELIABLE_MESSAGE_GAMESTATE );

	WriteState( outMsg );

	if( clientNum == -1 && forceTourneyHistory ) {
		forceTourneyHistory = false;
	}

	networkSystem->ServerSendReliableMessage( clientNum, outMsg );

	// don't update the state if we are working for a single client
	if ( clientNum == -1 ) {
		outMsg.ReadByte(); // pop off the msg ID
		ReceiveState( outMsg );
	}
}

/*
===============
rvTourneyGameState::WriteState
===============
*/
void rvTourneyGameState::WriteState( idBitMsg &msg ) {
	// send off base info
	rvGameState::PackState( msg );
	// add Tourney info
	PackState( msg );
}

/*
================
rvTourneyGameState::ReceiveState
================
*/
void rvTourneyGameState::ReceiveState( const idBitMsg& msg ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	rvGameState::UnpackState( msg );
	UnpackState( msg );

	if ( gameLocal.localClientNum >= 0 ) {
		GameStateChanged();
	}

	(rvTourneyGameState&)(*previousGameState) = (rvTourneyGameState&)(*this);
}

/*
================
rvTourneyGameState::SendInitialState
================
*/
void rvTourneyGameState::SendInitialState( int clientNum ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	rvTourneyGameState* previousState = (rvTourneyGameState*)previousGameState;

	rvTourneyGameState invalidState;

	previousGameState = &invalidState;

	
	// if the tourney has been going on, transmit the tourney history
	if( round > 0 ) {
		// comparing the tourney history for all state changes is wasteful when we really just want to send it to new clients
		packTourneyHistory = true;
	}
	
	SendState( clientNum );

	previousGameState = previousState;
}


/*
================
rvTourneyGameState::operator==
================
*/
bool rvTourneyGameState::operator==( const rvTourneyGameState& rhs ) const {
	assert( IsType( rvTourneyGameState::GetClassType() ) && rhs.IsType( rvTourneyGameState::GetClassType() ) );

	if( (rvGameState&)(*this) != (rvGameState&)rhs ) {
		return false;
	}

	if( round != rhs.round || startingRound != rhs.startingRound || maxRound != rhs.maxRound || tourneyState != rhs.tourneyState ) {
		return false;
	}

	for( int i = 0; i < MAX_ARENAS; i++ ) {
		if( arenas[ i ] != rhs.arenas[ i ] ) {
			return false;
		}
	}

	return true;
}

/*
================
rvTourneyGameState::operator=
================
*/
rvTourneyGameState& rvTourneyGameState::operator=( const rvTourneyGameState& rhs ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) && rhs.IsType( rvTourneyGameState::GetClassType() ) );

	(rvGameState&)(*this) = (rvGameState&)rhs;

	round = rhs.round;
	startingRound = rhs.startingRound;
	maxRound = rhs.maxRound;
	tourneyState = rhs.tourneyState;

	for( int i = 0; i < MAX_ARENAS; i++ ) {
		arenas[ i ] = rhs.arenas[ i ];
	}
	
	return (*this);
}

/*
================
rvTourneyGameState::GetNumArenas
Returns number of active arenas
================
*/
int rvTourneyGameState::GetNumArenas( void ) const {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	int num = 0;
	
	for( int i = 0; i < MAX_ARENAS; i++ ) {
		if( arenas[ i ].GetState() != AS_INACTIVE && arenas[ i ].GetState() != AS_DONE ) {
			num++;
		}
	}

	return num;
}

/*
================
rvTourneyGameState::SetupInitialBrackets

Sets up the brackets for a new match.  fragCount in playerstate is the player's
persistant frag count over an entire level.  In teamFragCount we store this
rounds score.

Initial bracket seeds are random
================
*/
void rvTourneyGameState::SetupInitialBrackets( void ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );
	idList<idPlayer*> unseededPlayers;
	int numRankedPlayers = gameLocal.mpGame.GetNumRankedPlayers();

	// all this crazy math does is figure out: 
	//	8 arenas to 4 rounds ( round 1: 8 arenas, round 2: 4 arenas, round 3: 2 arenas, round 4: 1 arena )
	//	16 arenas to 5 rounds ( round 1: 16 arenas, round 2: 8 arenas, round 3: 4 arenas, round 4: 2 arenas, round 5: 1 arena )
	//  etc
	int newMaxRound = idMath::ILog2( Max( idMath::CeilPowerOfTwo( MAX_ARENAS * 2 ), 2 ) );

	// we start at a round appropriate for our # of people
	// If you have 1-2 players, start in maxRound,  if you have 3-4 players start in maxRound - 1, if you have 5-8 players
	// start in maxRound - 2, if you have 9 - 16 players start in maxRound - 3, etc.
	int newRound = newMaxRound - idMath::ILog2( Max( idMath::CeilPowerOfTwo( gameLocal.mpGame.GetNumRankedPlayers() ), 2 ) ) + 1;

	round = newRound;
	maxRound = newMaxRound;
	startingRound = round;

	for( int i = 0; i < numRankedPlayers; i++ ) {
		unseededPlayers.Append( gameLocal.mpGame.GetRankedPlayer( i ) );
	}

	int numArenas = 0;

	while( unseededPlayers.Num() > 1 ) {
		idPlayer* playerOne = unseededPlayers[ gameLocal.random.RandomInt( unseededPlayers.Num() ) ];
		unseededPlayers.Remove( playerOne );

		idPlayer* playerTwo = unseededPlayers[ gameLocal.random.RandomInt( unseededPlayers.Num() ) ];
		unseededPlayers.Remove( playerTwo );

		rvTourneyArena& arena = arenas[ numArenas ];

		arena.Clear();
		arena.AddPlayers( playerOne, playerTwo );

		// place the players in the correct instance
		playerOne->JoinInstance( numArenas );
		playerTwo->JoinInstance( numArenas );

		playerOne->ServerSpectate( false );
		playerTwo->ServerSpectate( false );

		gameLocal.mpGame.SetPlayerTeamScore( playerOne, 0 );
		gameLocal.mpGame.SetPlayerTeamScore( playerTwo, 0 );

		gameLocal.Printf( "rvTourneyGameState::SetupInitialBrackets() - %s will face %s in arena %d\n", playerOne->GetUserInfo()->GetString( "ui_name" ), playerTwo->GetUserInfo()->GetString( "ui_name" ), numArenas );

		// this arena is ready to play
		arena.Ready();

		numArenas++;
	}

	assert( unseededPlayers.Num() == 0 || unseededPlayers.Num() == 1 );

	if( unseededPlayers.Num() ) {
		assert( unseededPlayers[ 0 ] );
		// the mid player gets a bye
		unseededPlayers[ 0 ]->SetTourneyStatus( PTS_UNKNOWN );
		unseededPlayers[ 0 ]->ServerSpectate( true );

		arenas[ numArenas ].AddPlayers( unseededPlayers[ 0 ], NULL );
		arenas[ numArenas ].Ready();

		// mark the ancestor arena of the bye player
		byeArena = numArenas * startingRound;
	} else {
		byeArena = -1;
	}
}

/*
================
rvTourneyGameState::ClientDisconnect
A player has disconnected from the server
================
*/
void rvTourneyGameState::ClientDisconnect( idPlayer* player ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );
	if( gameLocal.isClient ) {
		return;
	}
	
	// go through the tourney history and copy over the disconnecting player's name
	if( startingRound > 0 && round <= MAX_ROUNDS ) {
		for( int i = startingRound - 1; i < round - 1; i++ ) {
			for( int j = 0; j < MAX_ARENAS / (i + 1); j++ ) {
				if( tourneyHistory[ i ][ j ].playerOneNum == player->entityNumber ) {
					idStr::Copynz( tourneyHistory[ i ][ j ].playerOne, player->GetUserInfo()->GetString( "ui_name" ), MAX_TOURNEY_HISTORY_NAME_LEN );
					tourneyHistory[ i ][ j ].playerOneNum = -1;
				} else if( tourneyHistory[ i ][ j ].playerTwoNum == player->entityNumber ) {
					idStr::Copynz( tourneyHistory[ i ][ j ].playerTwo, player->GetUserInfo()->GetString( "ui_name" ), MAX_TOURNEY_HISTORY_NAME_LEN );
					tourneyHistory[ i ][ j ].playerTwoNum = -1;
				}
			}
		}

		// copy over the current rounds info if the player is playing
		for( int i = 0; i < MAX_ARENAS; i++ ) {
			if( arenas[ i ].GetPlayers()[ 0 ] == player ) {
				tourneyHistory[ round - 1 ][ i ].playerOneNum = -1;
				idStr::Copynz( tourneyHistory[ round - 1 ][ i ].playerOne, player->GetUserInfo()->GetString( "ui_name" ), MAX_TOURNEY_HISTORY_NAME_LEN );
				tourneyHistory[ round - 1 ][ i ].playerOneScore = gameLocal.mpGame.GetTeamScore( player );
			} else if( arenas[ i ].GetPlayers()[ 1 ] == player ) {
				tourneyHistory[ round - 1 ][ i ].playerTwoNum = -1;
				idStr::Copynz( tourneyHistory[ round - 1 ][ i ].playerTwo, player->GetUserInfo()->GetString( "ui_name" ), MAX_TOURNEY_HISTORY_NAME_LEN );
				tourneyHistory[ round - 1 ][ i ].playerTwoScore = gameLocal.mpGame.GetTeamScore( player );
			}
		}

		// retransmit tourney history to everyone
		packTourneyHistory = true;
	}

	RemovePlayer( player );
	
	// give RemovePlayer() a chance to update tourney history if needed
	if( packTourneyHistory ) {
		for( int i = 0; i < gameLocal.numClients; i++ ) {
			if( i == player->entityNumber ) {
				continue;
			}

			if( gameLocal.entities[ i ] ) {
				packTourneyHistory = true;
				SendState( i );
			}
		}
		packTourneyHistory = false;
	}
}

/*
================
rvTourneyGameState::Spectate
================
*/
void rvTourneyGameState::Spectate( idPlayer* player ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );
	assert( gameLocal.isServer );

	if( player->spectating && player->wantSpectate ) {
		RemovePlayer( player );
	}
}

/*
================
rvTourneyGameState::RemovePlayer
Removes the specified player from the arena
================
*/
void rvTourneyGameState::RemovePlayer( idPlayer* player ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	if( round < 1 || round > maxRound ) {
		return;
	}

	for( int i = 0; i < MAX_ARENAS; i++ ) {
		idPlayer** players = GetArenaPlayers( i );

		if( players[ 0 ] == player || players[ 1 ] == player ) {
			rvTourneyArena& arena = arenas[ i ];

			idPlayer* remainingPlayer = players[ 0 ] == player ? players[ 1 ] : players[ 0 ];

			bool arenaInProgress = arena.GetState() == AS_ROUND || arena.GetState() == AS_WARMUP;		
			bool remainingIsWinner = (remainingPlayer == arena.GetWinner());
			int remainingIndex = (remainingPlayer == arena.GetPlayers()[ 0 ]) ? 0 : 1;

			arena.RemovePlayer( player );

			// both players have disconnected from this arena, or the winner has disconnected, just return
			if( (!arena.GetPlayers()[ 0 ] && !arena.GetPlayers()[ 1 ]) || (!arenaInProgress && remainingPlayer && !remainingIsWinner) ) {
				arena.Clear();
				tourneyHistory[ round - 1 ][ i ].playerOneNum = -1;
				tourneyHistory[ round - 1 ][ i ].playerOne[ 0 ] = '\0';
				tourneyHistory[ round - 1 ][ i ].playerTwoNum = -1;
				tourneyHistory[ round - 1 ][ i ].playerTwo[ 0 ] = '\0';
				return;
			}

			assert( remainingPlayer );

			// if this arena is in progress, try restarting
			if( arenaInProgress && byeArena >= 0 && arenas[ byeArena / round ].GetWinner() ) {
				idPlayer* byePlayer = arenas[ byeArena / round ].GetWinner();

				// reset history for this new round
				tourneyHistory[ round - 1 ][ i ].playerOneNum = -1;
				tourneyHistory[ round - 1 ][ i ].playerOne[ 0 ] = '\0';
				tourneyHistory[ round - 1 ][ i ].playerTwoNum = -1;
				tourneyHistory[ round - 1 ][ i ].playerTwo[ 0 ] = '\0';

				arena.Clear();
				arenas[ byeArena / round ].Clear();

				arena.AddPlayers( remainingPlayer, byePlayer );

				// place the players in the correct instance
				remainingPlayer->JoinInstance( i );
				byePlayer->JoinInstance( i );

				remainingPlayer->ServerSpectate( false );
				byePlayer->ServerSpectate( false );

				gameLocal.mpGame.SetPlayerTeamScore( remainingPlayer, 0 );
				gameLocal.mpGame.SetPlayerTeamScore( byePlayer, 0 );

				gameLocal.Printf( "rvTourneyManager::RemovePlayer() - %s will face %s in arena %d\n", remainingPlayer->GetUserInfo()->GetString( "ui_name" ), byePlayer->GetUserInfo()->GetString( "ui_name" ), i );

				byeArena = -1;
				// this arena is ready to play
				arena.Ready();
			} else {
				// if the arena was in progress and didn't get a bye player to step in, this becomes a bye
				// arena - clear the tourney history
				if( arenaInProgress ) {
					tourneyHistory[ round - 1 ][ i ].playerOneNum = -1;
					tourneyHistory[ round - 1 ][ i ].playerOne[ 0 ] = '\0';
					tourneyHistory[ round - 1 ][ i ].playerOneScore = -1;

					tourneyHistory[ round - 1 ][ i ].playerTwoNum = -1;
					tourneyHistory[ round - 1 ][ i ].playerTwo[ 0 ] = '\0';
					tourneyHistory[ round - 1 ][ i ].playerTwoScore = -1;
				} else {
					// since the player is disconnecting, write the history for this round
					if( remainingIndex == 0 ) {
						tourneyHistory[ round - 1 ][ i ].playerOneNum = remainingPlayer->entityNumber;
						tourneyHistory[ round - 1 ][ i ].playerOne[ 0 ] = '\0';
						tourneyHistory[ round - 1 ][ i ].playerOneScore = gameLocal.mpGame.GetTeamScore( remainingPlayer );
					} else {
						tourneyHistory[ round - 1 ][ i ].playerTwoNum = remainingPlayer->entityNumber;
						tourneyHistory[ round - 1 ][ i ].playerTwo[ 0 ] = '\0';
						tourneyHistory[ round - 1 ][ i ].playerTwoScore = gameLocal.mpGame.GetTeamScore( remainingPlayer );
					}
				}
			}

			return;
		}
	}
}

int rvTourneyGameState::GetNextActiveArena( int arena ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	for( int i = arena + 1; i < MAX_ARENAS; i++ ) {
		if( arenas[ i ].GetState() != AS_INACTIVE && arenas[ i ].GetState() != AS_DONE ) {
			return i;
		}
	}

	for( int i = 0; i < arena; i++ ) {
		if( arenas[ i ].GetState() != AS_INACTIVE && arenas[ i ].GetState() != AS_DONE ) {
			return i;
		}
	}

	return arena;
}

int rvTourneyGameState::GetPrevActiveArena( int arena ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	for( int i = arena - 1; i >= 0; i-- ) {
		if( arenas[ i ].GetState() != AS_INACTIVE && arenas[ i ].GetState() != AS_DONE ) {
			return i;
		}
	}

	for( int i = MAX_ARENAS - 1; i > arena; i-- ) {
		if( arenas[ i ].GetState() != AS_INACTIVE && arenas[ i ].GetState() != AS_DONE ) {
			return i;
		}
	}

	return arena;
}

void rvTourneyGameState::SpectateCycleNext( idPlayer* player ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );
	assert( gameLocal.isServer );

	rvTourneyArena& spectatingArena = arenas[ player->GetArena() ];
	
	idPlayer** players = spectatingArena.GetPlayers();

	if( !players[ 0 ] || !players[ 1 ] || players[ 0 ]->spectating || players[ 1 ]->spectating ) {
		// setting the spectated client to ourselves will unlock us
		player->spectator = player->entityNumber;
		return;
	}

	if( player->spectator != players[ 0 ]->entityNumber && player->spectator != players[ 1 ]->entityNumber ) {
		player->spectator = players[ 0 ]->entityNumber;
	} else if( player->spectator == players[ 0 ]->entityNumber ) {
		player->spectator = players[ 1 ]->entityNumber;
	} else if( player->spectator == players[ 1 ]->entityNumber ) {
		if( gameLocal.time > player->lastArenaChange ) {
			if ( GetNumArenas() <= 0 ) {
				player->JoinInstance( 0 );
			} else {
				player->JoinInstance( GetNextActiveArena( player->GetArena() ) );
			}
			player->lastArenaChange = gameLocal.time + 2000;
			player->spectator = player->entityNumber;
		}
	}

	// this is where the listen server updates it's gui spectating elements
	if ( gameLocal.GetLocalPlayer() == player ) {
		rvTourneyArena& arena = arenas[ player->GetArena() ];

		if( arena.GetPlayers()[ 0 ] == NULL || arena.GetPlayers()[ 1 ] == NULL || (player->spectating && player->spectator != arena.GetPlayers()[ 0 ]->entityNumber && player->spectator != arena.GetPlayers()[ 1 ]->entityNumber) ) {
			gameLocal.mpGame.tourneyGUI.ArenaSelect( player->GetArena(), TGH_BRACKET );
		} else if( arena.GetPlayers()[ 0 ] == player || player->spectator == arena.GetPlayers()[ 0 ]->entityNumber ) {
			gameLocal.mpGame.tourneyGUI.ArenaSelect( player->GetArena(), TGH_PLAYER_ONE );
		} else if( arena.GetPlayers()[ 1 ] == player || player->spectator == arena.GetPlayers()[ 1 ]->entityNumber ) {
			gameLocal.mpGame.tourneyGUI.ArenaSelect( player->GetArena(), TGH_PLAYER_TWO );
		}

		gameLocal.mpGame.tourneyGUI.UpdateScores();
	}
}

void rvTourneyGameState::SpectateCyclePrev( idPlayer* player ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );
	assert( gameLocal.isServer );

	rvTourneyArena& spectatingArena = arenas[ player->GetArena() ];

	idPlayer** players = spectatingArena.GetPlayers();

	if( !players[ 0 ] || !players[ 1 ] || players[ 0 ]->spectating || players[ 1 ]->spectating ) {
		// setting the spectated client to ourselves will unlock us
		player->spectator = player->entityNumber;
		return;
	}

	if( player->spectator != players[ 0 ]->entityNumber && player->spectator != players[ 1 ]->entityNumber ) {
		if( gameLocal.time > player->lastArenaChange ) {
			if ( GetNumArenas() <= 0 ) {
				player->JoinInstance( 0 );
			} else {
				player->JoinInstance( GetPrevActiveArena( player->GetArena() ) );
			}
			player->lastArenaChange = gameLocal.time + 2000;
			
			rvTourneyArena& newSpectatingArena = arenas[ player->GetArena() ];
		
			idPlayer** newPlayers = newSpectatingArena.GetPlayers();

			if( !newPlayers[ 0 ] || !newPlayers[ 1 ] || newPlayers[ 0 ]->spectating || newPlayers[ 1 ]->spectating ) {
				// setting the spectated client to ourselves will unlock us
				player->spectator = player->entityNumber;
				return;
			}

			player->spectator = newPlayers[ 1 ]->entityNumber;
		} 
	} else if( player->spectator == players[ 0 ]->entityNumber ) {
		player->spectator = player->entityNumber;
	} else if( player->spectator == players[ 1 ]->entityNumber ) {
		player->spectator = players[ 0 ]->entityNumber;
	}

	// this is where the listen server updates it gui spectating elements
	if( gameLocal.GetLocalPlayer() == player ) {
		rvTourneyArena& arena = arenas[ player->GetArena() ];

		if( arena.GetPlayers()[ 0 ] == NULL || arena.GetPlayers()[ 1 ] == NULL || (player->spectating && player->spectator != arena.GetPlayers()[ 0 ]->entityNumber && player->spectator != arena.GetPlayers()[ 1 ]->entityNumber) ) {
			gameLocal.mpGame.tourneyGUI.ArenaSelect( player->GetArena(), TGH_BRACKET );
		} else if( arena.GetPlayers()[ 0 ] == player || player->spectator == arena.GetPlayers()[ 0 ]->entityNumber ) {
			gameLocal.mpGame.tourneyGUI.ArenaSelect( player->GetArena(), TGH_PLAYER_ONE );
		} else if( arena.GetPlayers()[ 1 ] == player || player->spectator == arena.GetPlayers()[ 1 ]->entityNumber ) {
			gameLocal.mpGame.tourneyGUI.ArenaSelect( player->GetArena(), TGH_PLAYER_TWO );
		}

		gameLocal.mpGame.tourneyGUI.UpdateScores();
	}
}

void rvTourneyGameState::UpdateTourneyBrackets( void ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	gameLocal.mpGame.tourneyGUI.SetupTourneyHistory( startingRound, round - 1, tourneyHistory );
}

void rvTourneyGameState::UpdateTourneyHistory( int round ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	for( int i = 0; i < MAX_ARENAS; i++ ) {
		// sometimes tourney history might have been updated for the current
		// round, so don't clobber any data

		if( (tourneyHistory[ round ][ i ].playerOne[ 0 ] != '\0' || tourneyHistory[ round ][ i ].playerOneNum != -1) ||
			(tourneyHistory[ round ][ i ].playerTwo[ 0 ] != '\0' || tourneyHistory[ round ][ i ].playerTwoNum != -1) ) {
			continue;
		}
		
		tourneyHistory[ round ][ i ].playerOne[ 0 ] = '\0';
		tourneyHistory[ round ][ i ].playerOneNum = -1;
		tourneyHistory[ round ][ i ].playerOneScore = 0;

		tourneyHistory[ round ][ i ].playerTwo[ 0 ] = '\0';
		tourneyHistory[ round ][ i ].playerTwoNum = -1;
		tourneyHistory[ round ][ i ].playerTwoScore = 0;

		if( arenas[ i ].GetState() == AS_DONE ) {
			idPlayer* playerOne = arenas[ i ].GetPlayers()[ 0 ];
			idPlayer* playerTwo = arenas[ i ].GetPlayers()[ 1 ];

			if( playerOne ) {
				tourneyHistory[ round ][ i ].playerOneNum = playerOne->entityNumber;
				tourneyHistory[ round ][ i ].playerOne[ MAX_TOURNEY_HISTORY_NAME_LEN - 1 ] = '\0';
				tourneyHistory[ round ][ i ].playerOneScore = gameLocal.mpGame.GetTeamScore( playerOne );
			}

			if( playerTwo ) {
				tourneyHistory[ round ][ i ].playerTwoNum = playerTwo->entityNumber;
				tourneyHistory[ round ][ i ].playerTwo[ MAX_TOURNEY_HISTORY_NAME_LEN - 1 ] = '\0';
				tourneyHistory[ round ][ i ].playerTwoScore = gameLocal.mpGame.GetTeamScore( playerTwo );
			}
		} 
	}
}


/*
================
rvTourneyGameState::FirstAvailableArena
Returns the first non-WARMUP arena available for use in the next round
================
*/
int rvTourneyGameState::FirstAvailableArena( void ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	for( int i = 0; i < MAX_ARENAS; i++ ) {
		if( arenas[ i ].GetState() != AS_WARMUP ) {
			return i;
		}
	}

	// no available arenas
	assert( 0 );
	return 0;
}

arenaOutcome_t* rvTourneyGameState::GetArenaOutcome( int arena ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	if( arenas[ arena ].GetState() != AS_DONE || (arenas[ arena ].GetPlayers()[ 0 ] && arenas[ arena ].GetPlayers()[ 1 ]) ) {
		return NULL;
	}

	return &(tourneyHistory[ round - 1 ][ arena ]);
}

bool rvTourneyGameState::HasBye( idPlayer* player ) {
	assert( IsType( rvTourneyGameState::GetClassType() ) );

	if( player == NULL || player->GetArena() < 0 || player->GetArena() >= MAX_ARENAS ) {
		return false;
	}

	// if we're one of the players in the arena we're in, and the other player is NULL, we have a bye
	if( (arenas[ player->GetArena() ].GetPlayers()[ 0 ] == player || arenas[ player->GetArena() ].GetPlayers()[ 1 ] == player) &&
		(arenas[ player->GetArena() ].GetPlayers()[ 0 ] == NULL || arenas[ player->GetArena() ].GetPlayers()[ 1 ] == NULL) ) {
		return true;
	}

	return false;
}

// simple RTTI
gameStateType_t rvGameState::type = GS_BASE;
gameStateType_t rvDMGameState::type = GS_DM;
gameStateType_t rvTeamDMGameState::type = GS_TEAMDM;
gameStateType_t rvCTFGameState::type = GS_CTF;
gameStateType_t rvTourneyGameState::type = GS_TOURNEY;

bool rvGameState::IsType( gameStateType_t type ) const {
	return ( type == rvGameState::type );
}

bool rvDMGameState::IsType( gameStateType_t type ) const {
	return ( type == rvDMGameState::type );
}

bool rvTeamDMGameState::IsType( gameStateType_t type ) const {
	return ( type == rvTeamDMGameState::type );
}

bool rvCTFGameState::IsType( gameStateType_t type ) const {
	return ( type == rvCTFGameState::type );
}

bool rvTourneyGameState::IsType( gameStateType_t type ) const {
	return ( type == rvTourneyGameState::type );
}

gameStateType_t rvGameState::GetClassType( void ) {
	return rvGameState::type;
}

gameStateType_t rvDMGameState::GetClassType( void ) {
	return rvDMGameState::type;
}

gameStateType_t rvTeamDMGameState::GetClassType( void ) {
	return rvTeamDMGameState::type;
}

gameStateType_t rvCTFGameState::GetClassType( void ) {
	return rvCTFGameState::type;
}

gameStateType_t rvTourneyGameState::GetClassType( void ) {
	return rvTourneyGameState::type;
}
