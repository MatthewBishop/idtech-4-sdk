
#pragma hdrstop
#include "../../idLib/precompiled.h"
#include "../Game_local.h"

/*
========================
idMenuWidget_NavButton::Update
========================
*/
void idMenuWidget_NavButton::Update() {

	if ( GetSprite() == NULL ) {
		return;
	}

	if ( btnLabel.IsEmpty() ) {
		GetSprite()->SetVisible( false );
		return;
	}

	GetSprite()->SetVisible( true );

	idSWFScriptObject * const spriteObject = GetSprite()->GetScriptObject();
	idSWFTextInstance * const text = spriteObject->GetNestedText( "txtVal" );
	if ( text != NULL ) {
		text->SetText( btnLabel.c_str() );
		text->SetStrokeInfo( true, 0.7f, 1.25f );
	}

	GetSprite()->SetXPos( xPos );

	if ( navState == NAV_WIDGET_SELECTED ) {
		idSWFSpriteInstance * backing = GetSprite()->GetScriptObject()->GetNestedSprite( "backing" );
		if ( backing != NULL && text != NULL  ) {
			backing->SetXPos( text->GetTextLength() + 53.0f );
		}
	}

	//
	// events
	//

	idSWFScriptObject * textObj = spriteObject->GetNestedObj( "txtVal" );

	if ( textObj != NULL ) {

		textObj->Set( "onPress", new ( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_PRESS, 0 ) );
		textObj->Set( "onRelease", new ( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_RELEASE, 0 ) );

		idSWFScriptObject * hitBox = spriteObject->GetObject( "hitBox" );
		if ( hitBox == NULL ) {
			hitBox = textObj;
		}

		hitBox->Set( "onRollOver", new ( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OVER, 0 ) );
		hitBox->Set( "onRollOut", new ( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OUT, 0 ) );
	}
}

/*
========================
idMenuWidget_NavButton::ExecuteEvent
========================
*/
bool idMenuWidget_NavButton::ExecuteEvent( const idWidgetEvent & event ) {
	
	bool handled = false;

	//// do nothing at all if it's disabled
	if ( GetState() != WIDGET_STATE_DISABLED ) {
		switch ( event.type ) {
			case WIDGET_EVENT_PRESS: {
				//AnimateToState( ANIM_STATE_DOWN );
				handled = true;
				break;
			}
			case WIDGET_EVENT_ROLL_OVER: {
				if ( GetMenuData() ) {
					GetMenuData()->PlaySound( GUI_SOUND_ROLL_OVER );
				}
				handled = true;
				break;
			}
		}
	}

	idMenuWidget::ExecuteEvent( event );

	return handled;
}

//*********************************************************************************************************
// idMenuWidget_MenuButton


/*
========================
idMenuWidget_NavButton::Update
========================
*/
void idMenuWidget_MenuButton::Update() {

	if ( GetSprite() == NULL ) {
		return;
	}

	if ( btnLabel.IsEmpty() ) {
		GetSprite()->SetVisible( false );
		return;
	}

	GetSprite()->SetVisible( true );

	idSWFScriptObject * const spriteObject = GetSprite()->GetScriptObject();
	idSWFTextInstance * const text = spriteObject->GetNestedText( "txtVal" );
	if ( text != NULL ) {
		text->SetText( btnLabel.c_str() );
		text->SetStrokeInfo( true, 0.7f, 1.25f );

		idSWFSpriteInstance * selBar = spriteObject->GetNestedSprite( "sel", "bar" );
		idSWFSpriteInstance * hoverBar =spriteObject->GetNestedSprite( "hover", "bar" );

		if ( selBar != NULL ) {
			selBar->SetXPos( text->GetTextLength() / 2.0f );
			selBar->SetScale( 100.0f * ( text->GetTextLength() / 300.0f ), 100.0f );
		}

		if ( hoverBar != NULL ) {
			hoverBar->SetXPos( text->GetTextLength() / 2.0f );
			hoverBar->SetScale( 100.0f * (  text->GetTextLength() / 352.0f ), 100.0f );
		}

		idSWFSpriteInstance * hitBox =spriteObject->GetNestedSprite( "hitBox" );
		if ( hitBox != NULL ) {
			hitBox->SetScale( 100.0f * ( text->GetTextLength() / 235 ), 100.0f );
		}
	}

	GetSprite()->SetXPos( xPos );
	
	idSWFScriptObject * textObj = spriteObject->GetNestedObj( "txtVal" );

	if ( textObj != NULL ) {

		textObj->Set( "onPress", new ( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_PRESS, 0 ) );
		textObj->Set( "onRelease", new ( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_RELEASE, 0 ) );

		idSWFScriptObject * hitBox = spriteObject->GetObject( "hitBox" );
		if ( hitBox == NULL ) {
			hitBox = textObj;
		}

		hitBox->Set( "onRollOver", new ( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OVER, 0 ) );
		hitBox->Set( "onRollOut", new ( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OUT, 0 ) );
	}
}
