
#pragma hdrstop
#include "../../idLib/precompiled.h"
#include "../Game_local.h"

void idMenuWidget_PDA_VideoInfo::Update() {

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

	idSWFTextInstance * txtHeading = GetSprite()->GetScriptObject()->GetNestedText( "txtName" );
	idSWFTextInstance * txtInfo = GetSprite()->GetScriptObject()->GetNestedText( "txtInfo" );
	
	int numVideos = player->GetInventory().videos.Num();
	if ( numVideos != 0 ) {
		const idDeclVideo * video = player->GetVideo( videoIndex );
		if( video != NULL ) {
			if ( txtHeading != NULL ) {
				txtHeading->SetText( video->GetVideoName() );
			}

			if ( txtInfo != NULL ) {
				txtInfo->SetText( video->GetInfo() );
			}
		}
	} else {
		if ( txtHeading != NULL ) {
			txtHeading->SetText( "" );
		}

		if ( txtInfo != NULL ) {
			txtInfo->SetText( "" );
		}
	}
}

void idMenuWidget_PDA_VideoInfo::ObserveEvent( const idMenuWidget & widget, const idWidgetEvent & event ) {
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
			videoIndex = list->GetViewIndex();

			idPlayer * player = gameLocal.GetLocalPlayer();
			if ( player != NULL ) {
				player->EndVideoDisk();

				const idDeclVideo * video = player->GetVideo( videoIndex );

				if ( video != NULL ) {
					idSWFSpriteInstance * videoSprite = GetSprite()->GetScriptObject()->GetNestedSprite( "video", "img" );
					if ( videoSprite != NULL ) {
						videoSprite->SetMaterial( video->GetPreview() );
					}
				}
			}			

			if ( GetParent() != NULL ) {
				idMenuScreen_PDA_VideoDisks * screen = dynamic_cast< idMenuScreen_PDA_VideoDisks * const >( GetParent() );
				if ( screen != NULL ) {
					screen->Update();
				}
			}

			break;
		}
	}
}

