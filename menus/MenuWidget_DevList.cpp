
#pragma hdrstop
#include "../precompiled.h"
#include "../MainMenuLocal.h"

/*
================================================================================================
idMenuWidget_DevList

Extends the standard list widget to support the developer menu.
================================================================================================
*/

namespace {
	/*
	================================================
	DevList_NavigateForward
	================================================
	*/
	class DevList_NavigateForward : public idSWFScriptFunction_RefCounted {
	public:
		DevList_NavigateForward( idMenuWidget_DevList * devList_, const int optionIndex_ ) :
			devList( devList_ ),
			optionIndex( optionIndex_ ) {

		}

		idSWFScriptVar Call( idSWFScriptObject * thisObject, const idSWFParmList & parms ) {
			devList->NavigateForward( optionIndex );
			return idSWFScriptVar();
		}

	private:
		idMenuWidget_DevList *	devList;
		int						optionIndex;
	};
}

/*
========================
idMenuWidget_DevList::Initialize
========================
*/
void idMenuWidget_DevList::Initialize() {
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN ).Set( WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_VERTICAL, 1 );
	AddEventAction( WIDGET_EVENT_SCROLL_UP ).Set( WIDGET_ACTION_START_REPEATER, WIDGET_ACTION_SCROLL_VERTICAL, -1 );
	AddEventAction( WIDGET_EVENT_SCROLL_DOWN_RELEASE ).Set( WIDGET_ACTION_STOP_REPEATER );
	AddEventAction( WIDGET_EVENT_SCROLL_UP_RELEASE ).Set( WIDGET_ACTION_STOP_REPEATER );

	int optionIndex = 0;
	while ( GetChildren().Num() < GetNumVisibleOptions() ) {
		idMenuWidget_Button * const buttonWidget = new ( TAG_MENU ) idMenuWidget_Button();
		buttonWidget->AddEventAction( WIDGET_EVENT_PRESS ).Set( new ( TAG_SWF ) DevList_NavigateForward( this, optionIndex++ ) );
		AddChild( buttonWidget );
	}
}

/*
========================
idMenuWidget_DevList::GetTotalNumberOfOptions
========================
*/
int idMenuWidget_DevList::GetTotalNumberOfOptions() const {
	if ( devMenuList == NULL ) {
		return 0;
	}

	return devMenuList->devMenuList.Num();
}

/*
========================
idMenuWidget_DevList::GoToFirstMenu
========================
*/
void idMenuWidget_DevList::GoToFirstMenu() {
	devMapListInfos.Clear();
	indexInfo_t & info = devMapListInfos.Alloc();
	info.name = "devmenuoption/main";

	devMenuList = NULL;

	RecalculateDevMenu();
}

/*
========================
idMenuWidget_DevList::NavigateForward
========================
*/
void idMenuWidget_DevList::NavigateForward( const int optionIndex ) {
	if ( devMenuList == NULL ) {
		return;
	}

	const int focusedIndex = GetViewOffset() + optionIndex;

	const idDeclDevMenuList::idDevMenuOption & devOption = devMenuList->devMenuList[ focusedIndex ];
	if ( ( devOption.devMenuDisplayName.Length() == 0 ) || ( devOption.devMenuDisplayName.Cmp( "..." ) == 0 ) ) {
		return;
	}

	if ( devOption.devMenuSubList != NULL ) {
		indexInfo_t & indexes = devMapListInfos.Alloc();
		indexes.name = devOption.devMenuSubList->GetName();
		indexes.focusIndex = GetFocusIndex();
		indexes.viewIndex = GetViewIndex();
		indexes.viewOffset = GetViewOffset();

		RecalculateDevMenu();

		SetViewIndex( 0 );
		SetViewOffset( 0 );

		Update();

		// NOTE: This must be done after the Update() because it MAY change the sprites that
		// children refer to
		GetChildByIndex( 0 ).SetState( WIDGET_STATE_SELECTED );
		ForceFocusIndex( 0 );
		SetFocusIndex( 0 );

		gameLocal->GetMainMenu()->ClearWidgetActionRepeater();
	} else {
		cmdSystem->AppendCommandText( va( "loadDevMenuOption %s %d\n", devMapListInfos[ devMapListInfos.Num() - 1 ].name, focusedIndex ) );
	}
}

/*
========================
idMenuWidget_DevList::NavigateBack
========================
*/
void idMenuWidget_DevList::NavigateBack() {
	assert( devMapListInfos.Num() != 0 );
	if ( devMapListInfos.Num() == 1 ) {
		// Important that this goes through as a DIRECT event, since more than likely the list
		// widget will have the parent's focus, so a standard ReceiveEvent() here would turn
		// into an infinite recursion.
		idWidgetEvent event( WIDGET_EVENT_BACK, 0, NULL, idSWFParmList() );
		
		idWidgetAction action;
		action.Set( WIDGET_ACTION_GO_BACK, MENU_ROOT );
		HandleAction( action, event );
		
		return;
	}

	// NOTE: we need a copy here, since it's about to be removed from the list
	const indexInfo_t indexes = devMapListInfos[ devMapListInfos.Num() - 1 ];
	assert( indexes.focusIndex < GetChildren().Num() );
	assert( ( indexes.viewIndex - indexes.viewOffset ) < GetNumVisibleOptions() );
	devMapListInfos.RemoveIndex( devMapListInfos.Num() - 1 );

	RecalculateDevMenu();

	SetViewIndex( indexes.viewIndex );
	SetViewOffset( indexes.viewOffset );

	Update();

	// NOTE: This must be done AFTER Update() because so that it is sure to refer to the proper sprite
	GetChildByIndex( indexes.focusIndex ).SetState( WIDGET_STATE_SELECTED );
	ForceFocusIndex( indexes.focusIndex );
	SetFocusIndex( indexes.focusIndex );

	gameLocal->GetMainMenu()->ClearWidgetActionRepeater();
}


/*
========================
idMenuWidget_DevList::RecalculateDevMenu
========================
*/
void idMenuWidget_DevList::RecalculateDevMenu() {
	if ( devMapListInfos.Num() > 0 ) {
		const idDeclDevMenuList * const devMenuListDecl = idDeclDevMenuList::resourceList.Load( devMapListInfos[ devMapListInfos.Num() - 1 ].name, false );
		if ( devMenuListDecl != NULL ) {
			devMenuList = devMenuListDecl;
		}
	}

	idSWFScriptObject & root = gameLocal->GetMainMenu()->GetSWF()->GetRootObject();
	for ( int i = 0; i < GetChildren().Num(); ++i ) {
		idMenuWidget & child = GetChildByIndex( i );
		child.SetSpritePath( GetSpritePath(), va( "option%d", i ) );
		if ( child.BindSprite( root ) ) {
			child.SetState( WIDGET_STATE_NORMAL );
			child.GetSprite()->StopFrame( 1 );
		}
	}
}
