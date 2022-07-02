// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../../framework/KeyInput.h"
#include "../../sys/sys_local.h"

#include "UIWindow.h"
#include "UserInterfaceLocal.h"
#include "UIEditW.h"


//===============================================================
//
//	sdUIEditW
//
//===============================================================

SD_UI_IMPLEMENT_CLASS( sdUIEditW, sdUIWindow )

SD_UI_PUSH_CLASS_TAG( sdUIEditW )
const char* sdUIEditW::eventNames[ EE_NUM_EVENTS - WE_NUM_EVENTS ] = {
	SD_UI_EVENT_TAG( "onInputFailed",		"", "Called if typing any characters that breaks the EF_ALLOW_DECIMAL/EF_INTEGERS_ONLY flags." ),
};
SD_UI_POP_CLASS_TAG

idHashMap< sdUITemplateFunction< sdUIEditW >* > sdUIEditW::editFunctions;
const char sdUITemplateFunctionInstance_IdentifierEditW[] = "sdUIEditWFunction";

/*
============
sdUIEditW::sdUIEditW
============
*/
sdUIEditW::sdUIEditW() {
	helper.Init( this );

	scriptState.GetProperties().RegisterProperty( "editText", editText );
	scriptState.GetProperties().RegisterProperty( "scrollAmount", scrollAmount );
	scriptState.GetProperties().RegisterProperty( "scrollAmountMax", scrollAmountMax );
	scriptState.GetProperties().RegisterProperty( "lineHeight", lineHeight );

	scriptState.GetProperties().RegisterProperty( "maxTextLength", maxTextLength );
	scriptState.GetProperties().RegisterProperty( "readOnly", readOnly );
	scriptState.GetProperties().RegisterProperty( "password", password );

	scriptState.GetProperties().RegisterProperty( "cursor", cursorName );
	scriptState.GetProperties().RegisterProperty( "overwriteCursor", overwriteCursorName );

	textAlignment.SetIndex( 0, TA_LEFT );
	maxTextLength = 0;
	readOnly = 0.0f;
	password = 0.0f;
	textOffset = idVec2( 2.0f, 0.0f );

	scrollAmount = vec2_zero;
	scrollAmountMax = 0.0f;
	lineHeight = 0.0f;

	scrollAmountMax.SetReadOnly( true );
	lineHeight.SetReadOnly( true );

	UI_ADD_WSTR_CALLBACK( editText, sdUIEditW, OnEditTextChanged )
	UI_ADD_FLOAT_CALLBACK( readOnly, sdUIEditW, OnReadOnlyChanged )
	UI_ADD_FLOAT_CALLBACK( flags, sdUIEditW, OnFlagsChanged )
	UI_ADD_STR_CALLBACK( cursorName, sdUIEditW, OnCursorNameChanged )
	UI_ADD_STR_CALLBACK( overwriteCursorName, sdUIEditW, OnOverwriteCursorNameChanged )

	SetWindowFlag( WF_ALLOW_FOCUS );
	SetWindowFlag( WF_CLIP_TO_RECT );
}

/*
============
sdUIEditW::InitProperties
============
*/
void sdUIEditW::InitProperties() {
	sdUIWindow::InitProperties();
	cursorName			= "editcursor";
	overwriteCursorName = "editcursor_overwrite";
}

/*
============
sdUIEditW::~sdUIEditW
============
*/
sdUIEditW::~sdUIEditW() {
	DisconnectGlobalCallbacks();
}

/*
============
sdUIEditW::FindFunction
============
*/
const sdUITemplateFunction< sdUIEditW >* sdUIEditW::FindFunction( const char* name ) {
	sdUITemplateFunction< sdUIEditW >** ptr;
	return editFunctions.Get( name, &ptr ) ? *ptr : NULL;
}

/*
============
sdUIEditW::GetFunction
============
*/
sdUIFunctionInstance* sdUIEditW::GetFunction( const char* name ) {
	const sdUITemplateFunction< sdUIEditW >* function = sdUIEditW::FindFunction( name );
	if ( !function ) {		
		return sdUIWindow::GetFunction( name );
	}

	return new sdUITemplateFunctionInstance< sdUIEditW, sdUITemplateFunctionInstance_IdentifierEditW >( this, function );
}

/*
============
sdUIEditW::RunNamedMethod
============
*/
bool sdUIEditW::RunNamedMethod( const char* name, sdUIFunctionStack& stack ) {
	const sdUITemplateFunction< sdUIEditW >* func = sdUIEditW::FindFunction( name );
	if ( !func ) {
		return sdUIWindow::RunNamedMethod( name, stack );
	}

	CALL_MEMBER_FN_PTR( this, func->GetFunction() )( stack );
	return true;
}

/*
============
sdUIEditW::DrawLocal
============
*/
void sdUIEditW::DrawLocal() {
	if( PreDraw() ) {
		DrawBackground( cachedClientRect );

		deviceContext->PushClipRect( GetDrawRect() );

		helper.DrawText( foreColor );
		helper.DrawLocal();

		deviceContext->PopClipRect();

		// border
		if ( borderWidth > 0.0f ) {
			deviceContext->DrawClippedBox( cachedClientRect.x, cachedClientRect.y, cachedClientRect.z, cachedClientRect.w, borderWidth, borderColor );
		}
	}

	PostDraw();	
}

/*
============
sdUIEditW::PostEvent
============
*/
bool sdUIEditW::PostEvent( const sdSysEvent* event ) {
	if ( !IsVisible() || !ParentsAllowEventPosting() ) {
		return false;
	}

	bool retVal = sdUIWindow::PostEvent( event );
	return helper.PostEvent( retVal, event );
}

/*
============
sdUIEditW::Script_ClearText
============
*/
void sdUIEditW::Script_ClearText( sdUIFunctionStack& stack ) {
	ClearText();	
}

/*
============
sdUIEditW::InitFunctions
============
*/
#pragma inline_depth( 0 )
#pragma optimize( "", off )
SD_UI_PUSH_CLASS_TAG( sdUIEditW )
void sdUIEditW::InitFunctions() {
	SD_UI_FUNC_TAG( clearText, "Clear all window text." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "clearText", new sdUITemplateFunction< sdUIEditW >( 'v', "", &sdUIEditW::Script_ClearText ) );

	SD_UI_FUNC_TAG( isWhitespace, "Check whitespaces, ignores all color codes." )
		SD_UI_FUNC_RETURN_PARM( float, "True if it is empty or contain only whitespaces." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "isWhitespace", new sdUITemplateFunction< sdUIEditW >( 'f', "", &sdUIEditW::Script_IsWhitespace ) );

	SD_UI_FUNC_TAG( insertText, "Insert text into the edit box." )
		SD_UI_FUNC_PARM( wstring, "text", "Text to insert." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "insertText", new sdUITemplateFunction< sdUIEditW >( 'v', "w", &sdUIEditW::Script_InsertText ) );

	SD_UI_FUNC_TAG( surroundSelection, "Select certain substring." )
		SD_UI_FUNC_PARM( wstring, "prefix", "Prefix to selection text." )
		SD_UI_FUNC_PARM( wstring, "suffix", "Suffix to selection text." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "surroundSelection", new sdUITemplateFunction< sdUIEditW >( 'v', "ww", &sdUIEditW::Script_SurroundSelection ) );

	SD_UI_FUNC_TAG( anySelected, "Check if any of the edit text is selected." )
		SD_UI_FUNC_RETURN_PARM( float, "True if any text has been selected." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "anySelected", new sdUITemplateFunction< sdUIEditW >( 'f', "", &sdUIEditW::Script_AnySelected ) );

	SD_UI_FUNC_TAG( selectAll, "Select all text in the edit box." )
	SD_UI_END_FUNC_TAG
	editFunctions.Set( "selectAll", new sdUITemplateFunction< sdUIEditW >( 'v', "", &sdUIEditW::Script_SelectAll ) );

	SD_UI_PUSH_GROUP_TAG( "Edit Flags" )

	SD_UI_ENUM_TAG( EF_INTEGERS_ONLY, "Accept only integer numbers in edit box." )
	sdDeclGUI::AddDefine( va( "EF_INTEGERS_ONLY %d", EF_INTEGERS_ONLY ) );
	SD_UI_ENUM_TAG( EF_ALLOW_DECIMAL, "Allow decimal numbers to be typed." )
	sdDeclGUI::AddDefine( va( "EF_ALLOW_DECIMAL %d", EF_ALLOW_DECIMAL ) );

	SD_UI_POP_GROUP_TAG
}
SD_UI_POP_CLASS_TAG
#pragma optimize( "", on )
#pragma inline_depth()

/*
============
sdUIEditW::ApplyLayout
============
*/
void sdUIEditW::ApplyLayout() {
	bool needLayout = windowState.recalculateLayout;
	sdUIWindow::ApplyLayout();
	if( needLayout ) {
		helper.TextChanged();
	}
}

/*
============
sdUIEditW::ClearText
============
*/
void sdUIEditW::ClearText() {
	helper.ClearText();
	editText = L"";
}

/*
============
sdUIEditW::OnEditTextChanged
============
*/
void sdUIEditW::OnEditTextChanged( const idWStr& oldValue, const idWStr& newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUIEditW::OnCursorNameChanged
============
*/
void sdUIEditW::OnCursorNameChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->LookupMaterial( newValue, cursor.mi, &cursor.width, &cursor.height );
}

/*
============
sdUIEditW::OnOverwriteCursorNameChanged
============
*/
void sdUIEditW::OnOverwriteCursorNameChanged( const idStr& oldValue, const idStr& newValue ) {
	GetUI()->LookupMaterial( newValue, overwriteCursor.mi, &overwriteCursor.width, &overwriteCursor.height );
}

/*
============
sdUIEditW::EndLevelLoad
============
*/
void sdUIEditW::EndLevelLoad() {
	sdUserInterfaceLocal::SetupMaterialInfo( cursor.mi, &cursor.width, &cursor.height );
	sdUserInterfaceLocal::SetupMaterialInfo( overwriteCursor.mi, &overwriteCursor.width, &overwriteCursor.height );
}



/*
============
sdUIEditW::EnumerateEvents
============
*/
void sdUIEditW::EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache ) {
	if ( !idStr::Icmp( name, "onInputFailed" ) ) {
		events.Append( sdUIEventInfo( EE_INPUT_FAILED, 0 ) );
		return;
	}
	sdUIWindow::EnumerateEvents( name, flags, events, tokenCache );
}

/*
============
sdUIEditW::OnReadOnlyChanged
============
*/
void sdUIEditW::OnReadOnlyChanged( const float oldValue, const float newValue ) {
	MakeLayoutDirty();
}

/*
============
sdUIEditW::OnFlagsChanged
============
*/
void sdUIEditW::OnFlagsChanged( const float oldValue, const float newValue ) {
	if( FlagChanged( oldValue, newValue, WF_COLOR_ESCAPES ) ) {
		MakeLayoutDirty();
	}
}

/*
============
sdUIEditW::Script_IsWhitespace
============
*/
void sdUIEditW::Script_IsWhitespace( sdUIFunctionStack& stack ) {
	for( int i = 0; i < editText.GetValue().Length(); i++ ) {
		const wchar_t& c = editText.GetValue()[ i ];
		if( idWStr::IsColor( &c ) ) {
			i++;
			continue;
		}
		
		if( c != L'\n' && c != L' ' ) {
			stack.Push( false );
			return;
		}
	}
	stack.Push( true );
}


/*
============
sdUIEditW::OnGainFocus
============
*/
void sdUIEditW::OnGainFocus( void ) {
	sdUIWindow::OnGainFocus();
	helper.OnGainFocus();
}

/*
============
sdUIEditW::Script_InsertText
============
*/
void sdUIEditW::Script_InsertText( sdUIFunctionStack& stack ) {
	idWStr text;
	stack.Pop( text );
	helper.InsertText( text.c_str() );
}

/*
============
sdUIEditW::Script_SurroundSelection
============
*/
void sdUIEditW::Script_SurroundSelection( sdUIFunctionStack& stack ) {
	idWStr prefix;
	stack.Pop( prefix );

	idWStr suffix;
	stack.Pop( suffix );

	helper.SurroundSelection( prefix.c_str(), suffix.c_str() );
}

/*
============
sdUIEditW::Script_AnySelected
============
*/
void sdUIEditW::Script_AnySelected( sdUIFunctionStack& stack ) {
	int selStart;
	int selEnd;
	helper.GetSelectionRange( selStart, selEnd );
	stack.Push( selStart != selEnd );
}


/*
============
sdUIEditW::Script_SelectAll
============
*/
void sdUIEditW::Script_SelectAll( sdUIFunctionStack& stack ) {
	helper.SelectAll();
}
