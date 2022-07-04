
#pragma hdrstop
#include "../../idLib/precompiled.h"
#include "../Game_local.h"

/*
========================
idMenuWidget_InfoBox::Update
========================
*/
void idMenuWidget_InfoBox::Initialize( idMenuHandler * data) {
	idMenuWidget::Initialize( data );
}

/*
========================
idMenuWidget_InfoBox::Update
========================
*/
void idMenuWidget_InfoBox::Update() {

	if ( GetSWFObject() == NULL ) {
		return;
	}

	idSWFScriptObject & root = GetSWFObject()->GetRootObject();
	if ( !BindSprite( root ) || GetSprite() == NULL ) {
		return;
	}

	idSWFTextInstance * txtHeading = GetSprite()->GetScriptObject()->GetNestedText( "info", "heading", "txtVal" );
	idSWFTextInstance * txtBody = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtBody" );

	if ( txtHeading != NULL ) {
		txtHeading->SetText( heading );
	}

	if ( txtBody != NULL ) {
		txtBody->SetText( info );
	}

	if ( scrollbar != NULL && txtBody != NULL ) {
		txtBody->CalcMaxScroll();
		scrollbar->Update();
	}

	idSWFScriptObject * info = GetSprite()->GetScriptObject()->GetNestedObj( "info" );
	if ( info != NULL ) {
		info->Set( "onRollOver", new ( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OVER, 0 ) );
		info->Set( "onRollOut", new ( TAG_SWF ) WrapWidgetSWFEvent( this, WIDGET_EVENT_ROLL_OUT, 0 ) );
	}

}

/*
========================
idMenuWidget_InfoBox::ObserveEvent
========================
*/
void idMenuWidget_InfoBox::ResetInfoScroll() {

	idSWFScriptObject & root = GetSWFObject()->GetRootObject();
	if ( !BindSprite( root ) || GetSprite() == NULL ){
		return;
	}

	idSWFTextInstance * txtBody = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtBody" );
	if ( txtBody != NULL ) {
		txtBody->scroll = 0;
	}

	if ( scrollbar != NULL ) {
		scrollbar->Update();
	}
}

/*
========================
idMenuWidget_InfoBox::Scroll
========================
*/
void idMenuWidget_InfoBox::Scroll(  int d ) {

	idSWFTextInstance * txtBody = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtBody" );

	if ( txtBody != NULL && txtBody->scroll + d >= 0 && txtBody->scroll + d <= txtBody->maxscroll ) {
		txtBody->scroll += d;
	}

	if ( scrollbar != NULL ) {
		scrollbar->Update();
	}

}

/*
========================
idMenuWidget_InfoBox::GetScroll
========================
*/
int	idMenuWidget_InfoBox::GetScroll() {

	idSWFTextInstance * txtBody = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtBody" );
	if ( txtBody != NULL ) {
		return txtBody->scroll;
	}

	return 0;
}

/*
========================
idMenuWidget_InfoBox::GetMaxScroll
========================
*/
int idMenuWidget_InfoBox::GetMaxScroll() {

	idSWFTextInstance * txtBody = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtBody" );
	if ( txtBody != NULL ) {
		return txtBody->maxscroll;
	}

	return 0;
}

/*
========================
idMenuWidget_InfoBox::SetScroll
========================
*/
void idMenuWidget_InfoBox::SetScroll( int scroll ) {

	idSWFTextInstance * txtBody = GetSprite()->GetScriptObject()->GetNestedText( "info", "txtBody" );

	if ( txtBody != NULL && scroll <= txtBody->maxscroll ) {
		txtBody->scroll = scroll;
	}

}

/*
========================
idMenuWidget_InfoBox::SetScrollbar
========================
*/
void idMenuWidget_InfoBox::SetScrollbar( idMenuWidget_ScrollBar * bar ) {
	scrollbar = bar;
}

/*
========================
idMenuWidget_InfoBox::ObserveEvent
========================
*/
bool idMenuWidget_InfoBox::HandleAction( idWidgetAction & action, const idWidgetEvent & event, idMenuWidget * widget, bool forceHandled ) {

	const idSWFParmList & parms = action.GetParms();

	if ( action.GetType() == WIDGET_ACTION_SCROLL_VERTICAL ) {
		const scrollType_t scrollType = static_cast< scrollType_t >( event.arg );
		if ( scrollType == SCROLL_SINGLE ) {
			Scroll( parms[ 0 ].ToInteger() );
		}
		return true;
	}

	return idMenuWidget::HandleAction( action, event, widget, forceHandled );
}

/*
========================
idMenuWidget_InfoBox::ObserveEvent
========================
*/
void idMenuWidget_InfoBox::ObserveEvent( const idMenuWidget & widget, const idWidgetEvent & event ) {
	switch ( event.type ) {
		case WIDGET_EVENT_FOCUS_ON: {
			ResetInfoScroll();
			break;
		}
	}
}