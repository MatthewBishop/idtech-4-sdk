
#pragma hdrstop
#include "../../idLib/precompiled.h"
#include "../Game_local.h"

/*
========================
idMenuWidget_MenuBar::Initialize
========================
*/
void idMenuWidget_MenuBar::Initialize( idMenuHandler * data ) {
	idMenuWidget::Initialize( data );
}

/*
========================
idMenuWidget_MenuBar::Update
========================
*/
void idMenuWidget_MenuBar::Update() {

	if ( GetSWFObject() == NULL ) {
		return;
	}

	idSWFScriptObject & root = GetSWFObject()->GetRootObject();

	if ( !BindSprite( root ) ) {
		return;
	}

	totalWidth = 0.0f;
	buttonPos = 0.0f;

	for ( int index = 0; index < GetNumVisibleOptions(); ++index ) {
			
		if ( index >= children.Num() ) {
			break;
		}

		if ( index != 0 ) {
			totalWidth += rightSpacer;
		}

		idMenuWidget & child = GetChildByIndex( index );
		child.SetSpritePath( GetSpritePath(), va( "btn%d", index ) );
		if ( child.BindSprite( root ) ) {
			PrepareListElement( child, index );
			child.Update();
		}
	}

	// 640 is half the size of our flash files width
	float xPos = 640.0f - ( totalWidth / 2.0f );
	GetSprite()->SetXPos( xPos );

	idSWFSpriteInstance * backing = GetSprite()->GetScriptObject()->GetNestedSprite( "backing" );
	if ( backing != NULL ) {
		if ( menuData != NULL && menuData->GetPlatform() != 2 ) {
			backing->SetVisible( false );
		} else {
			backing->SetVisible( true );
			backing->SetXPos( totalWidth / 2.0f );
		}
	}

}

/*
========================
idMenuWidget_MenuBar::SetListHeadings
========================
*/
void idMenuWidget_MenuBar::SetListHeadings( idList< idStr > & list ) {
	headings.Clear();
	for ( int index = 0; index < list.Num(); ++index ) {
		headings.Append( list[ index ] );
	}
}

/*
========================
idMenuWidget_MenuBar::GetTotalNumberOfOptions
========================
*/
int idMenuWidget_MenuBar::GetTotalNumberOfOptions() const { 
	return GetChildren().Num(); 
}

/*
========================
idMenuWidget_MenuBar::PrepareListElement
========================
*/
bool idMenuWidget_MenuBar::PrepareListElement( idMenuWidget & widget, const int navIndex ) {

	if ( navIndex >= GetNumVisibleOptions() ) {
		return false;
	}

	idMenuWidget_MenuButton * const button = dynamic_cast< idMenuWidget_MenuButton * >( &widget );
	if ( button == NULL || button->GetSprite() == NULL ) {
		return false;
	}

	if ( navIndex >= headings.Num() ) {
		button->SetLabel( "" );
	} else {
		button->SetLabel( headings[navIndex] );
		idSWFTextInstance * ti = button->GetSprite()->GetScriptObject()->GetNestedText( "txtVal" );
		if ( ti != NULL ) {
			ti->SetStrokeInfo( true, 0.7f, 1.25f );
			ti->SetText( headings[ navIndex ] );			
			button->SetPosition( buttonPos );
			totalWidth += ti->GetTextLength();
			buttonPos += rightSpacer + ti->GetTextLength();
		}
	}

	return true;
}
