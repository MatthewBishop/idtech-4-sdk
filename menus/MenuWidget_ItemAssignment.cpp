
#pragma hdrstop
#include "../../idLib/precompiled.h"
#include "../Game_local.h"

void idMenuWidget_ItemAssignment::SetIcon( int index, const idMaterial * icon ) {

	if ( index < 0 || index >= NUM_QUICK_SLOTS ) {
		return;
	}

	images[ index ] = icon;
}

void idMenuWidget_ItemAssignment::FindFreeSpot() {
	slotIndex = 0;
	for ( int i = 0; i < NUM_QUICK_SLOTS; ++i ) {
		if ( images[ i ] == NULL ) {
			slotIndex = i;
			break;
		}
	}	
}

/*
========================
idMenuWidget_ItemAssignment::Update
========================
*/
void idMenuWidget_ItemAssignment::Update() {

	if ( GetSWFObject() == NULL ) {
		return;
	}

	idSWFScriptObject & root = GetSWFObject()->GetRootObject();

	if ( !BindSprite( root ) ) {
		return;
	}

	idSWFSpriteInstance * dpad = GetSprite()->GetScriptObject()->GetNestedSprite( "dpad" );

	if ( dpad != NULL ) {
		dpad->StopFrame( slotIndex + 2 );
	}

	for ( int i = 0; i < NUM_QUICK_SLOTS; ++i ) {
		idSWFSpriteInstance * item = GetSprite()->GetScriptObject()->GetNestedSprite( va( "item%d", i ) );
		if ( item != NULL ) {
			if ( i == slotIndex ) {
				item->StopFrame( 2 );
			} else {
				item->StopFrame( 1 );
			}
		}

		idSWFSpriteInstance * itemIcon = GetSprite()->GetScriptObject()->GetNestedSprite( va( "item%d", i ), "img" );
		if ( itemIcon != NULL ) {
			if ( images[ i ] != NULL ) {
				itemIcon->SetVisible( true );
				itemIcon->SetMaterial( images[ i ] );
			} else {
				itemIcon->SetVisible( false );
			}
		}

		itemIcon = GetSprite()->GetScriptObject()->GetNestedSprite( va( "item%d", i ), "imgTop" );
		if ( itemIcon != NULL ) {
			if ( images[ i ] != NULL ) {
				itemIcon->SetVisible( true );
				itemIcon->SetMaterial( images[ i ] );
			} else {
				itemIcon->SetVisible( false );
			}
		}
	}
}
