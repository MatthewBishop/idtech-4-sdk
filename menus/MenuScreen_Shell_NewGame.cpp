
#pragma hdrstop
#include "../../idLib/precompiled.h"
#include "../Game_local.h"

const static int NUM_NEW_GAME_OPTIONS = 8;
/*
========================
idMenuScreen_Shell_NewGame::Initialize
========================
*/
void idMenuScreen_Shell_NewGame::Initialize( idMenuHandler * data ) {
	idMenuScreen::Initialize( data );

	if ( data != NULL ) {
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuNewGame" );

	options = new  (TAG_SWF) idMenuWidget_DynamicList();
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;	
	idList< idStr > option;
	option.Append( "#str_swf_doom3" );	// doom 3
	menuOptions.Append( option );
	option.Clear();
	option.Append( "#str_swf_resurrection" );	// resurrection of evil
	menuOptions.Append( option );
	option.Clear();
	option.Append( "#str_swf_lost_episodes" );	// lost episodes
	menuOptions.Append( option );

	options->SetListData( menuOptions );
	options->SetNumVisibleOptions( NUM_NEW_GAME_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );

	while ( options->GetChildren().Num() < NUM_NEW_GAME_OPTIONS ) {
		idMenuWidget_Button * const buttonWidget = new  (TAG_SWF) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PRESS_FOCUSED, options->GetChildren().Num() );
		buttonWidget->Initialize( data );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );

	AddChild( options );

	btnBack = new  (TAG_SWF) idMenuWidget_Button();
	btnBack->Initialize( data );
	btnBack->SetLabel( "#str_swf_campaign" );
	btnBack->SetSpritePath( GetSpritePath(), "info", "btnBack" );
	btnBack->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_GO_BACK );

	AddChild( btnBack );

	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( new (TAG_SWF) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( new (TAG_SWF) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( new (TAG_SWF) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( new (TAG_SWF) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK ).Set( new (TAG_SWF) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_DOWN_START_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK ).Set( new (TAG_SWF) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_SCROLL_UP_START_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ).Set( new (TAG_SWF) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_DOWN_LSTICK_RELEASE ) );
	options->AddEventAction( WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ).Set( new (TAG_SWF) idWidgetActionHandler( options, WIDGET_ACTION_EVENT_STOP_REPEATER, WIDGET_EVENT_SCROLL_UP_LSTICK_RELEASE ) );
}

/*
========================
idMenuScreen_Shell_NewGame::Update
========================
*/
void idMenuScreen_Shell_NewGame::Update() {

	if ( menuData != NULL ) {
		idMenuWidget_CommandBar * cmdBar = menuData->GetCmdBar();
		if ( cmdBar != NULL ) {
			cmdBar->ClearAllButtons();
			idMenuWidget_CommandBar::buttonInfo_t * buttonInfo;			
			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY2 );
			if ( menuData->GetPlatform() != 2 ) {
				buttonInfo->label = "#str_00395";
			}
			buttonInfo->action.Set( WIDGET_ACTION_GO_BACK );

			buttonInfo = cmdBar->GetButton( idMenuWidget_CommandBar::BUTTON_JOY1 );
			if ( menuData->GetPlatform() != 2 ) {
				buttonInfo->label = "#str_SWF_SELECT";
			}
			buttonInfo->action.Set( WIDGET_ACTION_PRESS_FOCUSED );
		}		
	}

	idSWFScriptObject & root = GetSWFObject()->GetRootObject();
	if ( BindSprite( root ) ) {
		idSWFTextInstance * heading = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtHeading" );
		if ( heading != NULL ) {
			heading->SetText( "#str_02207" );	// NEW GAME
			heading->SetStrokeInfo( true, 0.75f, 1.75f );
		}

		idSWFSpriteInstance * gradient = GetSprite()->GetScriptObject()->GetNestedSprite( "info", "gradient" );
		if ( gradient != NULL && heading != NULL ) {
			gradient->SetXPos( heading->GetTextLength() );
		}
	}

	if ( btnBack != NULL ) {
		btnBack->BindSprite( root );
	}

	idMenuScreen::Update();
}

/*
========================
idMenuScreen_Shell_NewGame::ShowScreen
========================
*/
void idMenuScreen_Shell_NewGame::ShowScreen( const mainMenuTransition_t transitionType ) {
	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_NewGame::HideScreen
========================
*/
void idMenuScreen_Shell_NewGame::HideScreen( const mainMenuTransition_t transitionType ) {
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_NewGame::HandleAction h
========================
*/
bool idMenuScreen_Shell_NewGame::HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled ) {

	if ( menuData != NULL ) {
		if ( menuData->ActiveScreen() != SHELL_AREA_NEW_GAME ) {
			return false;
		}
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList & parms = action.GetParms();

	switch ( actionType ) {
		case WIDGET_ACTION_GO_BACK: {
			if ( menuData != NULL ) {
				menuData->SetNextScreen( SHELL_AREA_CAMPAIGN, MENU_TRANSITION_SIMPLE );
			}
			return true;
		}
		case WIDGET_ACTION_PRESS_FOCUSED: {
			if ( options == NULL ) {
				return true;
			}

			int selectionIndex = options->GetViewIndex();
			if ( parms.Num() == 1 ) {
				selectionIndex = parms[0].ToInteger();
			}			

			if ( selectionIndex != options->GetFocusIndex() ) {
				options->SetViewIndex( selectionIndex );
				options->SetFocusIndex( selectionIndex );
			}

			idMenuHandler_Shell * shell = dynamic_cast< idMenuHandler_Shell * >( menuData );
			if ( shell != NULL ) {
				shell->SetNewGameType( selectionIndex );
				menuData->SetNextScreen( SHELL_AREA_DIFFICULTY, MENU_TRANSITION_SIMPLE );
			}

			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}