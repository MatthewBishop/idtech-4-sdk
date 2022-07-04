
#pragma hdrstop
#include "../../idLib/precompiled.h"
#include "../Game_local.h"

/*
========================
idMenuWidget_NavBar::Initialize
========================
*/
void idMenuWidget_NavBar::Initialize( idMenuHandler * data ) {
	idMenuWidget::Initialize( data );
}

/*
========================
idMenuWidget_NavBar::PrepareListElement
========================
*/
void idMenuWidget_NavBar::Update() {

	if ( GetSWFObject() == NULL ) {
		return;
	}

	idSWFScriptObject & root = GetSWFObject()->GetRootObject();

	if ( !BindSprite( root ) ) {
		return;
	}
	
	int rightIndex = 0;	

	buttonPos = initialPos;

	for ( int index = 0; index < GetNumVisibleOptions() - 1; ++index ) {
		idSWFSpriteInstance * const rightOption = GetSprite()->GetScriptObject()->GetSprite( va( "optionRight%d", index ) );
		rightOption->SetVisible( false );
		idSWFSpriteInstance * const leftOption = GetSprite()->GetScriptObject()->GetSprite( va( "optionLeft%d", index ) );
		leftOption->SetVisible( false );
	}

	for ( int index = 0; index < GetTotalNumberOfOptions(); ++index ) {
		idMenuWidget & child = GetChildByIndex( index );
		idMenuWidget_NavButton * const button = dynamic_cast< idMenuWidget_NavButton * >( &child );
		button->SetLabel( "" );
	}

	for ( int index = 0; index < GetNumVisibleOptions(); ++index ) {
		if ( index < GetFocusIndex() ) {
			idMenuWidget & child = GetChildByIndex( index );
			child.SetSpritePath( GetSpritePath(), va( "optionLeft%d", index ) );

			if ( child.BindSprite( root ) ) {
				PrepareListElement( child, index );
				child.Update();
			}

		} else if ( index > GetFocusIndex() ) {			
			int rightChildIndex = ( GetNumVisibleOptions() - 1 ) + ( index - 1 );
			idMenuWidget & child = GetChildByIndex( rightChildIndex );
			child.SetSpritePath( GetSpritePath(), va( "optionRight%d", rightIndex ) );
			rightIndex++;

			if ( child.BindSprite( root ) ) {
				PrepareListElement( child, index );
				child.Update();
			}

		} else {
			int mainIndex = GetTotalNumberOfOptions() - 1;
			idMenuWidget & child = GetChildByIndex( mainIndex );
			child.SetSpritePath( GetSpritePath(), va( "option" ) );

			if ( child.BindSprite( root ) ) {
				PrepareListElement( child, index );
				child.Update();
			}
		}		
	}
}

/*
========================
idMenuWidget_NavBar::SetListHeadings
========================
*/
void idMenuWidget_NavBar::SetListHeadings( idList< idStr > & list ) {
	headings.Clear();
	for ( int index = 0; index < list.Num(); ++index ) {
		headings.Append( list[ index ] );
	}
}

/*
========================
idMenuWidget_NavBar::GetTotalNumberOfOptions
========================
*/
int idMenuWidget_NavBar::GetTotalNumberOfOptions() const { 
	return GetChildren().Num(); 
}

/*
========================
idMenuWidget_NavBar::PrepareListElement
========================
*/
bool idMenuWidget_NavBar::PrepareListElement( idMenuWidget & widget, const int navIndex ) {

	if ( navIndex >= GetNumVisibleOptions() || navIndex >= headings.Num() ) {
		return false;
	}

	idMenuWidget_NavButton * const button = dynamic_cast< idMenuWidget_NavButton * >( &widget );
	if ( button == NULL || button->GetSprite() == NULL ) {
		return false;
	}

	button->SetLabel( headings[navIndex] );
	idSWFTextInstance * ti = button->GetSprite()->GetScriptObject()->GetNestedText( "txtVal" );
	if ( ti != NULL ) {
		ti->SetStrokeInfo( true, 0.7f, 1.25f );
		if ( navIndex < GetFocusIndex() ) {
			ti->SetText( headings[ navIndex ] );			
			buttonPos = buttonPos + ti->GetTextLength();
			button->SetPosition( buttonPos );
			button->SetNavIndex( navIndex, idMenuWidget_NavButton::NAV_WIDGET_LEFT );
			buttonPos += leftSpacer;
		} else if ( navIndex > GetFocusIndex() ) {
			ti->SetText( headings[ navIndex ] );
			ti->SetStrokeInfo( true, 0.7f, 1.25f );
			button->GetSprite()->SetXPos( buttonPos );
			button->SetPosition( buttonPos );
			button->SetNavIndex( navIndex, idMenuWidget_NavButton::NAV_WIDGET_RIGHT );
			buttonPos = buttonPos + ti->GetTextLength() + rightSpacer;			
		} else {
			ti->SetText( headings[ navIndex ] );
			ti->SetStrokeInfo( true, 0.7f, 1.25f );
			button->GetSprite()->SetXPos( buttonPos );
			button->SetPosition( buttonPos );
			button->SetNavIndex( navIndex, idMenuWidget_NavButton::NAV_WIDGET_SELECTED );
			buttonPos = buttonPos + ti->GetTextLength() + selectedSpacer;
		}
	}

	return true;

}
