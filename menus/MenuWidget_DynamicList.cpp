
#pragma hdrstop
#include "../../idLib/precompiled.h"
#include "../Game_local.h"

/*
========================
idMenuWidget_DynamicList::Initialize
========================
*/
void idMenuWidget_DynamicList::Initialize( idMenuHandler * data ) {
	idMenuWidget::Initialize( data );
}

/*
========================
idMenuWidget_DynamicList::Update
========================
*/
void idMenuWidget_DynamicList::Update() {

	if ( GetSWFObject() == NULL ) {
		return;
	}

	idSWFScriptObject & root = GetSWFObject()->GetRootObject();

	if ( !BindSprite( root ) ) {
		return;
	}

	for ( int optionIndex = 0; optionIndex < GetNumVisibleOptions(); ++optionIndex ) {
		
		if ( optionIndex >= children.Num() ) {
			idSWFSpriteInstance * item = GetSprite()->GetScriptObject()->GetNestedSprite( va( "item%d", optionIndex ) );
			if ( item != NULL ) {
				item->SetVisible( false );
				continue;
			}
		}

		idMenuWidget & child = GetChildByIndex( optionIndex );
		const int childIndex = GetViewOffset() + optionIndex;
		bool shown = false;
		child.SetSpritePath( GetSpritePath(), va( "item%d", optionIndex ) );
		if ( child.BindSprite( root ) ) {
			
			if ( optionIndex >= GetTotalNumberOfOptions() ) {
				child.ClearSprite();
				continue;
			} else {
				//const int controlIndex = GetNumVisibleOptions() - Min( GetNumVisibleOptions(), GetTotalNumberOfOptions() ) + optionIndex;
				shown = PrepareListElement( child, childIndex );
				child.Update();
			}

			if ( !shown ) {
				child.SetState( WIDGET_STATE_HIDDEN );
			} else {
				if ( optionIndex == focusIndex ) {
					child.SetState( WIDGET_STATE_SELECTING );
				} else {
					child.SetState( WIDGET_STATE_NORMAL );
				}
			}
		}
	}

	idSWFSpriteInstance * const upSprite = GetSprite()->GetScriptObject()->GetSprite( "upIndicator" );
	if ( upSprite != NULL ) {
		upSprite->SetVisible( GetViewOffset() > 0 );
	}

	idSWFSpriteInstance * const downSprite = GetSprite()->GetScriptObject()->GetSprite( "downIndicator" );
	if ( downSprite != NULL ) {
		downSprite->SetVisible( GetViewOffset() + GetNumVisibleOptions() < GetTotalNumberOfOptions() );
	}

}

/*
========================
idMenuWidget_DynamicList::GetTotalNumberOfOptions
========================
*/
int idMenuWidget_DynamicList::GetTotalNumberOfOptions() const {

	if ( controlList ) {
		return GetChildren().Num();
	}

	return listItemInfo.Num();
}

/*
========================
idMenuWidget_DynamicList::PrepareListElement
========================
*/
bool idMenuWidget_DynamicList::PrepareListElement( idMenuWidget & widget, const int childIndex ) {

	idMenuWidget_ScoreboardButton * const sbButton = dynamic_cast< idMenuWidget_ScoreboardButton * >( &widget );
	if ( sbButton != NULL ) {
		return true;
	}

	if ( listItemInfo.Num() == 0 ) {
		return true;
	}

	if ( childIndex > listItemInfo.Num() ) {
		return false;
	}

	idMenuWidget_Button * const button = dynamic_cast< idMenuWidget_Button * >( &widget );
	if ( button != NULL ) {
		button->SetIgnoreColor( ignoreColor );
		button->SetValues( listItemInfo[ childIndex ] );
		if ( listItemInfo[ childIndex ].Num() > 0 ) {
			return true;
		} 
	}
	
	return false;
}

/*
========================
idMenuWidget_DynamicList::SetListData
========================
*/
void idMenuWidget_DynamicList::SetListData( idList< idList< idStr, TAG_IDLIB_LIST_MENU >, TAG_IDLIB_LIST_MENU > & list ) {
	listItemInfo.Clear();
	for ( int i = 0; i < list.Num(); ++i ) {
		idList< idStr > values;
		for ( int j = 0; j < list[i].Num(); ++j ) {
			values.Append( list[i][j] );
		}
		listItemInfo.Append( values );
	}
}

/*
========================
idMenuWidget_DynamicList::Recalculate
========================
*/
void idMenuWidget_DynamicList::Recalculate() {

	idSWF * swf = GetSWFObject();

	if ( swf == NULL ) {
		return;
	}

	idSWFScriptObject & root = swf->GetRootObject();
	for ( int i = 0; i < GetChildren().Num(); ++i ) {
		idMenuWidget & child = GetChildByIndex( i );
		child.SetSpritePath( GetSpritePath(), "info", "list", va( "item%d", i ) );
		if ( child.BindSprite( root ) ) {
			child.SetState( WIDGET_STATE_NORMAL );
			child.GetSprite()->StopFrame( 1 );
		}
	}
}

/*
========================
idMenuWidget_ScoreboardList::Update
========================
*/
void idMenuWidget_ScoreboardList::Update() {

	if ( GetSWFObject() == NULL ) {
		return;
	}

	idSWFScriptObject & root = GetSWFObject()->GetRootObject();

	if ( !BindSprite( root ) ) {
		return;
	}

	for ( int optionIndex = 0; optionIndex < GetNumVisibleOptions(); ++optionIndex ) {
		idMenuWidget & child = GetChildByIndex( optionIndex );
		const int childIndex = GetViewOffset() + optionIndex;
		bool shown = false;
		child.SetSpritePath( GetSpritePath(), va( "item%d", optionIndex ) );
		if ( child.BindSprite( root ) ) {
			shown = PrepareListElement( child, childIndex );

			if ( optionIndex == focusIndex && child.GetState() != WIDGET_STATE_SELECTED && child.GetState() != WIDGET_STATE_SELECTING ) {
				child.SetState( WIDGET_STATE_SELECTING );
			} else if ( optionIndex != focusIndex && child.GetState() != WIDGET_STATE_NORMAL ) {
				child.SetState( WIDGET_STATE_NORMAL );
			}

			child.Update();
		}
	}
}

/*
========================
idMenuWidget_ScoreboardList::GetTotalNumberOfOptions
========================
*/
int idMenuWidget_ScoreboardList::GetTotalNumberOfOptions() const {
	return GetChildren().Num();
}