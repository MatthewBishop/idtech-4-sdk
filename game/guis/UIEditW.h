// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_GUIS_USERINTERFACEEDITW_H__
#define __GAME_GUIS_USERINTERFACEEDITW_H__

#include "UserInterfaceTypes.h"
#include "UIEditHelper.h"

//===============================================================
//
//	sdUIEditW
//
//===============================================================

extern const char sdUITemplateFunctionInstance_IdentifierEditW[];

SD_UI_PUSH_CLASS_TAG( sdUIEditW )
SD_UI_CLASS_INFO_TAG(
/* ============ */
	"The wide edit window type has the same functionality as the regular edit window, but it supports wide strings for non ASCII input. " \
	"It is mainly used when typing messages to friends or in the chat window.\n\n" \

	"Templates are used to decrease the amount of text needed to create an edit box, the _editw/_editw_scroll and _end_edit/_end_editw_scroll templates should be used."
/* ============ */
)
SD_UI_CLASS_EXAMPLE_TAG(
/* ============ */
    " _editw_scroll( MessageMemberMessage, 0, 0, 100, 60 )\n" \
        " \tproperties {\n" \
        	" \t\tvec2 textAlignment = TA_LEFT, TA_VCENTER;\n" \
			" \t\tfloat	maxTextLength = 512;\n" \
        " \t}\n" \
    " _end_editw_scroll\n"
/* ============ */
)
SD_UI_POP_CLASS_TAG
SD_UI_PROPERTY_TAG(
alias = "editw";
)
class sdUIEditW :
	public sdUIWindow {
friend class sdUIEditHelper< sdUIEditW, idWStr, wchar_t >;
public:
	SD_UI_DECLARE_CLASS( sdUIEditW )

	enum eEditFlag {
		EF_INTEGERS_ONLY	= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 0 >::VALUE,
		EF_ALLOW_DECIMAL	= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 1 >::VALUE,
		EF_UPPERCASE		= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 2 >::VALUE,
		EF_MULTILINE		= BITT< sdUIWindow::NEXT_WINDOW_FLAG + 3 >::VALUE,
	};

	typedef enum editEvent_e {
		EE_INPUT_FAILED = WE_NUM_EVENTS + 1,
		EE_NUM_EVENTS,
	} editEvent_t;
	
												sdUIEditW();
		virtual									~sdUIEditW();

		virtual void							InitProperties();
		virtual const char*						GetScopeClassName() const { return "sdUIEditW"; }

		virtual bool							PostEvent( const sdSysEvent* event );

		virtual sdUIFunctionInstance*			GetFunction( const char* name );
		virtual bool							RunNamedMethod( const char* name, sdUIFunctionStack& stack );
		void									Script_ClearText( sdUIFunctionStack& stack );
		void									Script_IsWhitespace( sdUIFunctionStack& stack );
		void									Script_InsertText( sdUIFunctionStack& stack );
		void									Script_SurroundSelection( sdUIFunctionStack& stack );
		void									Script_AnySelected( sdUIFunctionStack& stack );
		void									Script_SelectAll( sdUIFunctionStack& stack );

		virtual void							OnGainFocus( void );

		sdBounds2D								GetDrawRect() const;


		static void										InitFunctions( void );
		static void										ShutdownFunctions( void ) { editFunctions.DeleteContents(); }
		static const sdUITemplateFunction< sdUIEditW >*	FindFunction( const char* name );

		virtual void							EnumerateEvents( const char* name, const idList<unsigned short>& flags, idList< sdUIEventInfo >& events, const idTokenCache& tokenCache );
		virtual int								GetMaxEventTypes( void ) const { return EE_NUM_EVENTS; }
		virtual const char*						GetEventNameForType( int event ) const { return ( event < ( WE_NUM_EVENTS + 1 )) ? sdUIWindow::GetEventNameForType( event ): eventNames[ event - WE_NUM_EVENTS - 1 ]; }


protected:
	virtual void							DrawLocal();
	virtual void							ApplyLayout();
	virtual void							EndLevelLoad();

private:
	void									ClearText();
	static bool								CharIsPrintable( wchar_t c ) { return c > 0x20; }

	void									OnEditTextChanged( const idWStr& oldValue, const idWStr& newValue );
	void									OnReadOnlyChanged( const float oldValue, const float newValue );
	void									OnCursorNameChanged( const idStr& oldValue, const idStr& newValue );
	void									OnOverwriteCursorNameChanged( const idStr& oldValue, const idStr& newValue );
	void									OnFlagsChanged( const float oldValue, const float newValue );

protected:
	SD_UI_PROPERTY_TAG(
	title				= "Object/Edit Text";
	desc				= "Editable text displayed within the object.";
	editor				= "editText";
	datatype			= "wstring";
	)
	sdWStringProperty	editText;

	SD_UI_PROPERTY_TAG(
	title				= "Object/Max Length";
	desc				= "Maximum allowed length for the text";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty	maxTextLength;

	SD_UI_PROPERTY_TAG(
	title				= "Object/Read Only";
	desc				= "Don't allow editing of the text";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty	readOnly;

	SD_UI_PROPERTY_TAG(
	title				= "Object/Password";
	desc				= "Display text as a series of *s";
	editor				= "edit";
	datatype			= "float";
	)
	sdFloatProperty	password;

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Cursor";
	desc				= "Normal editing mode cursor";
	editor				= "edit";
	datatype			= "String";
	)
	sdStringProperty	cursorName;

	SD_UI_PROPERTY_TAG(
	title				= "Drawing/Overwrite Cursor";
	desc				= "Insert mode cursor";
	editor				= "edit";
	datatype			= "String";
	)
	sdStringProperty	overwriteCursorName;

	sdVec2Property		scrollAmount;
	sdFloatProperty		scrollAmountMax;
	sdFloatProperty		lineHeight;

protected:
	static idHashMap< sdUITemplateFunction< sdUIEditW >* >	editFunctions;
	static const char* eventNames[ EE_NUM_EVENTS - WE_NUM_EVENTS ];

private:
	uiDrawPart_t cursor;
	uiDrawPart_t overwriteCursor;

	sdUIEditHelper< sdUIEditW, idWStr, wchar_t >	helper;
};

/*
============
sdUIEditW::GetDrawRect
============
*/
ID_INLINE sdBounds2D sdUIEditW::GetDrawRect() const {
	sdBounds2D rect( GetWorldRect() );
	rect.TranslateSelf( textOffset.GetValue() );
	rect.GetMaxs() -= ( textOffset.GetValue() * 2.0f );
	return rect;
}

#endif // ! __GAME_GUIS_USERINTERFACEEDITW_H__
