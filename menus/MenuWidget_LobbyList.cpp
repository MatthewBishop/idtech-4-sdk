
#pragma hdrstop
#include "../../idLib/precompiled.h"
#include "../Game_local.h"

/*
========================
idMenuWidget_LobbyList::Update
========================
*/
void idMenuWidget_LobbyList::Update() {

	if ( GetSWFObject() == NULL ) {
		return;
	}

	idSWFScriptObject & root = GetSWFObject()->GetRootObject();

	if ( !BindSprite( root ) ) {
		return;
	}

	for ( int i = 0; i < headings.Num(); ++i ) {
		idSWFTextInstance * txtHeading = GetSprite()->GetScriptObject()->GetNestedText( va( "heading%d", i ) );
		if ( txtHeading != NULL ) {
			txtHeading->SetText( headings[i] );
			txtHeading->SetStrokeInfo( true, 0.75f, 1.75f );
		}
	}

	for ( int optionIndex = 0; optionIndex < GetNumVisibleOptions(); ++optionIndex ) {		
		bool shown = false;
		if ( optionIndex < GetChildren().Num() ) {
			idMenuWidget & child = GetChildByIndex( optionIndex );
			child.SetSpritePath( GetSpritePath(), va( "item%d", optionIndex ) );
			if ( child.BindSprite( root ) ) {
				shown = PrepareListElement( child, optionIndex );
				if ( shown ) {
					child.GetSprite()->SetVisible( true );
					child.Update();
				} else {
					child.GetSprite()->SetVisible( false );
				}
			}
		}
	}
}

/*
========================
idMenuWidget_LobbyList::PrepareListElement
========================
*/
bool idMenuWidget_LobbyList::PrepareListElement( idMenuWidget & widget, const int childIndex ) {

	idMenuWidget_LobbyButton * const button = dynamic_cast< idMenuWidget_LobbyButton * >( &widget );
	if ( button == NULL ) {
		return false;
	}

	if ( !button->IsValid() ) {
		return false;
	}

	return true;

}

/*
========================
idMenuWidget_LobbyList::SetHeadingInfo
========================
*/
void idMenuWidget_LobbyList::SetHeadingInfo( idList< idStr > & list ) {
	headings.Clear();
	for ( int index = 0; index < list.Num(); ++index ) {
		headings.Append( list[ index ] );
	}
}

/*
========================
idMenuWidget_LobbyList::SetEntryData
========================
*/
void idMenuWidget_LobbyList::SetEntryData( int index, idStr name, voiceStateDisplay_t voiceState ) {

	if ( GetChildren().Num() == 0 || index >= GetChildren().Num() ) {
		return;
	}

	idMenuWidget & child = GetChildByIndex( index );
	idMenuWidget_LobbyButton * const button = dynamic_cast< idMenuWidget_LobbyButton * >( &child );

	if ( button == NULL ) {
		return;
	}

	button->SetButtonInfo( name, voiceState );
}