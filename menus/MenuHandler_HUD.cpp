
#pragma hdrstop
#include "../../idLib/precompiled.h"
#include "../Game_local.h"

static const int TIP_DISPLAY_TIME = 5000;

/*
========================
idMenuHandler_HUD::Update
========================
*/
void idMenuHandler_HUD::Update() {

	if ( gui == NULL || !gui->IsActive() ) {
		return;
	}

	if ( nextScreen != activeScreen ) {
	
		if ( activeScreen > HUD_AREA_INVALID && activeScreen < HUD_NUM_AREAS && menuScreens[ activeScreen ] != NULL ) {
			menuScreens[ activeScreen ]->HideScreen( static_cast<mainMenuTransition_t>(transition) );
		}

		if ( nextScreen > HUD_AREA_INVALID && nextScreen < HUD_NUM_AREAS && menuScreens[ nextScreen ] != NULL ) {
			menuScreens[ nextScreen ]->ShowScreen( static_cast<mainMenuTransition_t>(transition) );			
		}

		transition = MENU_TRANSITION_INVALID;
		activeScreen = nextScreen;
	}

	idPlayer * player = gameLocal.GetLocalPlayer();
	if ( player != NULL ) {
		if ( player->IsTipVisible() && autoHideTip && !hiding ) {		
			if ( gameLocal.time >= tipStartTime + TIP_DISPLAY_TIME ) {
				player->HideTip();
			}
		}

		if ( player->IsSoundChannelPlaying( SND_CHANNEL_PDA_AUDIO ) && GetHud() != NULL ) {
			GetHud()->UpdateAudioLog( true );
		} else {
			GetHud()->UpdateAudioLog( false );
		}

		if ( radioMessage ) {
			GetHud()->UpdateCommunication( true, player );
		} else {
			GetHud()->UpdateCommunication( false, player );
		}

	}

	idMenuHandler::Update();
}

/*
========================
idMenuHandler_HUD::ActivateMenu
========================
*/
void idMenuHandler_HUD::ActivateMenu( bool show ) {

	idMenuHandler::ActivateMenu( show );
	
	idPlayer * player = gameLocal.GetLocalPlayer();
	if ( player == NULL ) {
		return;
	}  	

	if ( show ) {
		activeScreen = HUD_AREA_INVALID;
		nextScreen = HUD_AREA_PLAYING;
	} else {
		activeScreen = HUD_AREA_INVALID;
		nextScreen = HUD_AREA_INVALID;
	}

}

/*
========================
idMenuHandler_HUD::Initialize
========================
*/
void idMenuHandler_HUD::Initialize( const char * swfFile, idSoundWorld * sw ) {
	idMenuHandler::Initialize( swfFile, sw );

	//---------------------
	// Initialize the menus
	//---------------------
#define BIND_HUD_SCREEN( screenId, className, menuHandler )				\
	menuScreens[ (screenId) ] = new className();						\
	menuScreens[ (screenId) ]->Initialize( menuHandler );				\
	menuScreens[ (screenId) ]->AddRef();

	for ( int i = 0; i < HUD_NUM_AREAS; ++i ) {
		menuScreens[ i ] = NULL;
	}

	BIND_HUD_SCREEN( HUD_AREA_PLAYING, idMenuScreen_HUD, this );
}

/*
========================
idMenuHandler_HUD::GetMenuScreen
========================
*/
idMenuScreen * idMenuHandler_HUD::GetMenuScreen( int index ) {

	if ( index < 0 || index >= HUD_NUM_AREAS ) {
		return NULL;
	}

	return menuScreens[ index ];

}

/*
========================
idMenuHandler_HUD::GetHud
========================
*/
idMenuScreen_HUD * idMenuHandler_HUD::GetHud() {
	idMenuScreen_HUD * screen = dynamic_cast< idMenuScreen_HUD * >( menuScreens[ HUD_AREA_PLAYING ] );
	return screen;
}

/*
========================
idMenuHandler_HUD::ShowTip
========================
*/
void idMenuHandler_HUD::ShowTip( const char * title, const char * tip, bool autoHide ) {	
	autoHideTip = autoHideTip;
	tipStartTime = gameLocal.time;
	hiding = false;
	idMenuScreen_HUD * screen = GetHud();
	if ( screen != NULL ) {
		screen->ShowTip( title, tip );
	}
}

/*
========================
idMenuHandler_HUD::HideTip
========================
*/
void idMenuHandler_HUD::HideTip() {
	idMenuScreen_HUD * screen = GetHud();
	if ( screen != NULL && !hiding ) {
		screen->HideTip();
	}
	hiding = true;
}