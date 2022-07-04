
#pragma hdrstop
#include "../../idLib/precompiled.h"
#include "../Game_local.h"

static const int MAX_EMAIL_ITEMS = 7;

/*
========================
idMenuWidget_PDA_EmailInbox::Initialize
========================
*/
void idMenuWidget_PDA_EmailInbox::Initialize( idMenuHandler * data ) {

	idMenuWidget_ScrollBar * scrollbar = new (TAG_SWF) idMenuWidget_ScrollBar();
	scrollbar->SetSpritePath( GetSpritePath(), "info", "scrollbar" );
	scrollbar->Initialize( data );

	emailList = new (TAG_SWF) idMenuWidget_DynamicList();
	emailList->SetSpritePath( GetSpritePath(), "info", "options" );
	emailList->SetNumVisibleOptions( MAX_EMAIL_ITEMS );
	emailList->SetWrappingAllowed( true );
	while ( emailList->GetChildren().Num() < MAX_EMAIL_ITEMS ) {
		idMenuWidget_Button * const buttonWidget = new (TAG_SWF) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( WIDGET_ACTION_PDA_SELECT_EMAIL, emailList->GetChildren().Num() );
		buttonWidget->AddEventAction( WIDGET_EVENT_FOCUS_ON ).Set( WIDGET_ACTION_REFRESH );
		buttonWidget->Initialize( data );
		buttonWidget->RegisterEventObserver( scrollbar );
		emailList->AddChild( buttonWidget );
	}
	emailList->Initialize( data );
	emailList->AddChild( scrollbar );

	AddChild( emailList );
}

/*
========================
idMenuWidget_PDA_EmailInbox::Update
========================
*/
void idMenuWidget_PDA_EmailInbox::Update() {

	if ( GetSWFObject() == NULL ) {
		return;
	}

	idSWFScriptObject & root = GetSWFObject()->GetRootObject();
	if ( !BindSprite( root ) || GetSprite() == NULL ) {
		return;
	}

	idPlayer * player = gameLocal.GetLocalPlayer();
	if ( player == NULL ) {
		return;
	}

	if ( pdaIndex > player->GetInventory().pdas.Num() ) {
		return;
	}

	const idDeclPDA * pda = player->GetInventory().pdas[ pdaIndex ];

	idSWFScriptObject * dataObj = GetSprite()->GetScriptObject()->GetNestedObj( "info" );
	if ( dataObj != NULL && pda != NULL ) {

		idSWFTextInstance * txtOwner = dataObj->GetNestedText( "heading", "txtOwner" );
		if ( txtOwner != NULL ) {
			idStr ownerText = idLocalization::GetString( "#str_01474" );
			ownerText.Append( ": ");

			if ( pdaIndex == 0 ) {
				ownerText.Append( session->GetLocalUserName(0) );
			} else {
				ownerText.Append( pda->GetFullName() );
			}
			txtOwner->SetText( ownerText.c_str() );

			if ( pdaIndex == 0 ) {
				txtOwner->SetIgnoreColor( false );
			} else {
				txtOwner->SetIgnoreColor( false );
				txtOwner->SetText( pda->GetFullName() );
			}

		}

		if ( emailList != NULL ) {
			const idDeclEmail *email = NULL;
			emailInfo.Clear();
			for ( int index = 0; index < pda->GetNumEmails(); ++index ) {
				idList< idStr > emailData;
				email = pda->GetEmailByIndex( index );						
				if ( email != NULL ) {
					emailData.Append( email->GetFrom() );
					emailData.Append( email->GetSubject() );
					emailData.Append( email->GetDate() );
				}
				emailInfo.Append( emailData );
			}

			emailList->SetListData( emailInfo );
			if ( emailList->BindSprite( root ) ) {
				emailList->Update();
			}
		}
	}
}

/*
========================
idMenuWidget_PDA_EmailInbox::ObserveEvent
========================
*/
void idMenuWidget_PDA_EmailInbox::ObserveEvent( const idMenuWidget & widget, const idWidgetEvent & event ) {
	const idMenuWidget_Button * const button = dynamic_cast< const idMenuWidget_Button * >( &widget );
	if ( button == NULL ) {
		return;
	}

	const idMenuWidget * const listWidget = button->GetParent();

	if ( listWidget == NULL ) {
		return;
	}

	switch ( event.type ) {
		case WIDGET_EVENT_FOCUS_ON: {
			const idMenuWidget_DynamicList * const list = dynamic_cast< const idMenuWidget_DynamicList * const >( listWidget );
			int oldIndex = pdaIndex;
			if ( list != NULL ) {
				pdaIndex = list->GetViewIndex();
				Update();
			}
			if ( emailList != NULL && oldIndex != pdaIndex ) {
				emailList->SetFocusIndex( 0 );
				emailList->SetViewIndex( 0 );
				emailList->SetViewOffset( 0 );
			}			
			break;
									}
		case WIDGET_EVENT_FOCUS_OFF: {
			Update();
			if ( emailList != NULL ) {
				emailList->SetFocusIndex( 0 );
				emailList->SetViewIndex( 0 );
				emailList->SetViewOffset( 0 );
			}
			break;
		}
	}
}