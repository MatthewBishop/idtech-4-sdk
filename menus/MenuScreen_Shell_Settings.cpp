
#pragma hdrstop
#include "../../idLib/precompiled.h"
#include "../Game_local.h"

const static int NUM_SETTING_OPTIONS = 8;

enum settingMenuCmds_t {
	SETTING_CMD_CONTROLS,
	SETTING_CMD_GAMEPLAY,
	SETTING_CMD_SYSTEM,
	SETTING_CMD_3D,
};

/*
========================
idMenuScreen_Shell_Settings::Initialize
========================
*/
void idMenuScreen_Shell_Settings::Initialize( idMenuHandler * data ) {
	idMenuScreen::Initialize( data );

	if ( data != NULL ) {
		menuGUI = data->GetGUI();
	}

	SetSpritePath( "menuSettings" );

	options = new (TAG_SWF) idMenuWidget_DynamicList();
	idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > menuOptions;	
	idList< idStr > option;

	option.Append( "#str_04158" );	// controls
	menuOptions.Append( option );
	option.Clear();
	option.Append( "#str_02401" );	// game options
	menuOptions.Append( option );
	option.Clear();
	option.Append( "#str_04160" );	// system
	menuOptions.Append( option );
	option.Clear();

	if ( renderSystem->IsStereoScopicRenderingSupported() ) {
		option.Append( "#str_swf_stereoscopics" );	// Stereoscopic Rendering
		menuOptions.Append( option );
	}

	options->SetListData( menuOptions );
	options->SetNumVisibleOptions( NUM_SETTING_OPTIONS );
	options->SetSpritePath( GetSpritePath(), "info", "options" );
	options->SetWrappingAllowed( true );
	AddChild( options );

	idMenuWidget_Help * const helpWidget = new ( TAG_SWF ) idMenuWidget_Help();
	helpWidget->SetSpritePath( GetSpritePath(), "info", "helpTooltip" );
	AddChild( helpWidget );

	const char * tips[] = { "#str_02166", "#str_02168", "#str_02170", "#str_swf_customize_3d" };

	while ( options->GetChildren().Num() < NUM_SETTING_OPTIONS ) {
		idMenuWidget_Button * const buttonWidget = new (TAG_SWF) idMenuWidget_Button();
		buttonWidget->Initialize( data );
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_COMMAND, options->GetChildren().Num() );

		if ( options->GetChildren().Num() < menuOptions.Num() ) {
			buttonWidget->SetDescription( tips[options->GetChildren().Num()] );
		}

		buttonWidget->RegisterEventObserver( helpWidget );
		options->AddChild( buttonWidget );
	}
	options->Initialize( data );

	btnBack = new (TAG_SWF) idMenuWidget_Button();
	btnBack->Initialize( data );	
	idMenuHandler_Shell * handler = dynamic_cast< idMenuHandler_Shell * >( data );
	if ( handler != NULL && handler->GetInGame() ) {
		btnBack->SetLabel( "#str_swf_pause_menu" );
	} else {
		btnBack->SetLabel( "#str_02305" );
	}
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
idMenuScreen_Shell_Settings::Update
========================
*/
void idMenuScreen_Shell_Settings::Update() {

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
			heading->SetText( "#str_swf_settings" );
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
idMenuScreen_Shell_Settings::ShowScreen
========================
*/
void idMenuScreen_Shell_Settings::ShowScreen( const mainMenuTransition_t transitionType ) {
	idMenuScreen::ShowScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Settings::HideScreen
========================
*/
void idMenuScreen_Shell_Settings::HideScreen( const mainMenuTransition_t transitionType ) {
	idMenuScreen::HideScreen( transitionType );
}

/*
========================
idMenuScreen_Shell_Settings::HandleAction h
========================
*/
bool idMenuScreen_Shell_Settings::HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled ) {

	if ( menuData == NULL ) {
		return true;
	}

	if ( menuData->ActiveScreen() != SHELL_AREA_SETTINGS ) {
		return false;
	}

	widgetAction_t actionType = action.GetType();
	const idSWFParmList & parms = action.GetParms();

	switch ( actionType ) {
		case WIDGET_ACTION_GO_BACK: {
			menuData->SetNextScreen( SHELL_AREA_ROOT, MENU_TRANSITION_SIMPLE );
			return true;
		}
		case WIDGET_ACTION_COMMAND: {
			switch ( parms[0].ToInteger() ) {
				case SETTING_CMD_CONTROLS: {
					menuData->SetNextScreen( SHELL_AREA_CONTROLS, MENU_TRANSITION_SIMPLE );
					break;	
				}
				case SETTING_CMD_GAMEPLAY: {
					menuData->SetNextScreen( SHELL_AREA_GAME_OPTIONS, MENU_TRANSITION_SIMPLE );
					break;
				}
				case SETTING_CMD_SYSTEM: {
					menuData->SetNextScreen( SHELL_AREA_SYSTEM_OPTIONS, MENU_TRANSITION_SIMPLE );
					break;
				}
				case SETTING_CMD_3D: {
					menuData->SetNextScreen( SHELL_AREA_STEREOSCOPICS, MENU_TRANSITION_SIMPLE );
					break;
				}
			}

			if ( options != NULL ) {
				int selectionIndex = options->GetViewIndex();
				if ( parms.Num() == 1 ) {
					selectionIndex = parms[0].ToInteger();
				}	

				if ( options->GetFocusIndex() != selectionIndex ) {
					options->SetFocusIndex( selectionIndex );
					options->SetViewIndex( options->GetViewOffset() + selectionIndex );
				}
			}

			return true;
		}
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}