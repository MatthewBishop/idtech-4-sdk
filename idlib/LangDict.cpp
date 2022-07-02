// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop


/*
============
idLangDict::idLangDict
============
*/
idLangDict::idLangDict( void ) {
	args.SetGranularity( 256 );
	hash.SetGranularity( 256 );
	hash.Clear( 4096, 8192 );
	baseID = 0;
}

/*
============
idLangDict::~idLangDict
============
*/
idLangDict::~idLangDict( void ) {
	Clear();
}

/*
============
idLangDict::Clear
============
*/
void idLangDict::Clear( void ) {
	args.Clear();
	hash.Clear();
}

/*
============
idLangDict::Load
============
*/
bool idLangDict::Load( const char* fileName, bool clear ) {
	if ( clear ) {
		Clear();
	}

	idFile* file = idLib::fileSystem->OpenFileRead( fileName );
	if ( file == NULL ) {
		return false;
	}

	sdUTF8 utf8( file );

	fileSystem->CloseFile( file );

	int numChars = utf8.DecodeLength();
	wchar_t* ucs2 = static_cast< wchar_t* >( Mem_Alloc( ( numChars + 1 ) * sizeof( wchar_t ) ) );

	utf8.Decode( ucs2 );

	idWLexer src( ucs2, idWStr::Length( ucs2 ), fileName );
	if ( !src.IsLoaded() ) {
		Mem_Free( ucs2 );
		return false;
	}

	idWToken tok, tok2;
	src.ExpectTokenString( L"{" );
	while ( src.ReadToken( &tok ) ) {
		if ( tok == L"}" ) {
			break;
		}
		if ( src.ReadToken( &tok2 ) ) {
			if ( tok2 == L"}" ) {
				break;
			}
			idLangKeyValue kv;
			kv.key = va( "%ls", tok.c_str() );	// FIXME: do this nicer?
			if( kv.key.Cmpn( STRTABLE_ID, STRTABLE_ID_LENGTH ) != 0 ) {
				src.Warning( "Bad string id '%s' found", kv.key.c_str() );
				src.SkipRestOfLine();
				continue;
			}
			if( FindKeyValue( kv.key.c_str() ) != NULL ) {
				src.Warning( "Duplicate string id '%s' found", kv.key.c_str() );
			}
			kv.value = tok2;
			hash.Add( GetHashKey( kv.key ), args.Append( kv ) );
		}
	}
	idLib::common->Printf( "%i strings read from %s\n", args.Num(), fileName );

	Mem_Free( ucs2 );

	return true;
}

/*
============
idLangDict::Save
============
*/
void idLangDict::Save( const char *fileName ) {
	/*idFile *outFile = idLib::fileSystem->OpenFileWrite( fileName );
	outFile->WriteFloatString( "// string table\n// english\n//\n\n{\n" );
	for ( int j = 0; j < args.Num(); j++ ) {
		outFile->WriteFloatString( "\t\"%s\"\t\"", args[j].key.c_str() );
		int l = args[j].value.Length();
		char slash = '\\';
		char tab = 't';
		char nl = 'n';
		for ( int k = 0; k < l; k++ ) {
			char ch = args[j].value[k];
			if ( ch == '\t' ) {
				outFile->Write( &slash, 1 );
				outFile->Write( &tab, 1 );
			} else if ( ch == '\n' || ch == '\r' ) {
				outFile->Write( &slash, 1 );
				outFile->Write( &nl, 1 );
			} else {
				outFile->Write( &ch, 1 );
			}
		}
		outFile->WriteFloatString( "\"\n" );
	}
	outFile->WriteFloatString( "\n}\n" );
	idLib::fileSystem->CloseFile( outFile );*/
}

/*
============
idLangDict::FindKeyValue
============
*/
const idLangKeyValue* idLangDict::FindKeyValue( const char* str ) const {
	if ( str == NULL || str[0] == '\0' ) {
		return NULL;
	}
	int hashKey = GetHashKey( str );
	for ( int i = hash.GetFirst( hashKey ); i != -1; i = hash.GetNext( i ) ) {
		if ( args[i].key.Cmp( str ) == 0 ) {
			return &args[i];
		}
	}
	return NULL;
}

/*
============
idLangDict::GetString
============
*/
const wchar_t* idLangDict::GetString( const char *str ) const {
	if ( str == NULL || str[0] == '\0' ) {
		return L"";
	}

	int hashKey = GetHashKey( str );
	for ( int i = hash.GetFirst( hashKey ); i != -1; i = hash.GetNext( i ) ) {
		if ( args[i].key.Cmp( str ) == 0 ) {
			return args[i].value.c_str();
		}
	}

	idLib::common->Warning( "Unknown String ID '%s'", str );
	return va( L"###Bad String %hs###", str );
}

/*
============
idLangDict::AddString
============
*/
const char* idLangDict::AddString( const wchar_t *str ) {
	
	if ( ExcludeString( str ) ) {
		return "###Excluded String###";
	}

	int c = args.Num();
	for ( int j = 0; j < c; j++ ) {
		if ( args[j].value.Cmp( str ) == 0 ) {
			return args[j].key;
		}
	}

	int id = GetNextId();
	idLangKeyValue kv;
	kv.key = va( "#str_%05i", id );
	kv.value = str;
	c = args.Append( kv );
	assert( kv.key.Cmpn( STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 );
	hash.Add( GetHashKey( kv.key ), c );
	return args[c].key;
}

/*
============
idLangDict::GetNumKeyVals
============
*/
int idLangDict::GetNumKeyVals( void ) const {
	return args.Num();
}

/*
============
idLangDict::GetKeyVal
============
*/
const idLangKeyValue* idLangDict::GetKeyVal( int i ) const {
	return &args[i];
}

/*
============
idLangDict::AddKeyVal
============
*/
void idLangDict::AddKeyVal( const char *key, const wchar_t *val ) {
	idLangKeyValue kv;
	kv.key = key;
	kv.value = val;
	assert( kv.key.Cmpn( STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 );
	hash.Add( GetHashKey( kv.key ), args.Append( kv ) );
}

/*
============
idLangDict::ExcludeString
============
*/
bool idLangDict::ExcludeString( const wchar_t *str ) const {
	if ( str == NULL ) {
		return true;
	}

	int c = idWStr::Length( str );
	if ( c <= 1 ) {
		return true;
	}

	if ( idWStr::Cmpn( str, LSTRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 ) {
		return true;
	}

	if ( idWStr::Icmpn( str, L"gui::", idWStr::Length( L"gui::" ) ) == 0 ) {
		return true;
	}

	if ( str[0] == L'$' ) {
		return true;
	}

	int i;
	for ( i = 0; i < c; i++ ) {
		if ( isalpha( str[i] ) ) {
			break;
		}
	}
	if ( i == c ) {
		return true;
	}
	
	return false;
}

/*
============
idLangDict::GetNextId
============
*/
int idLangDict::GetNextId( void ) const {
	int c = args.Num();

	// Let an external user supply the base id for this dictionary
	int id = baseID;

	if ( c == 0 ) {
		return id;
	}

	idStr work;
	for ( int j = 0; j < c; j++ ) {
		work = args[j].key;
		work.StripLeading( STRTABLE_ID );
		int test = atoi( work );
		if ( test > id ) {
			id = test;
		}
	}
	return id + 1;
}

/*
============
idLangDict::GetHashKey
============
*/
int idLangDict::GetHashKey( const char *str ) const {
	int hashKey = 0;
	for ( str += STRTABLE_ID_LENGTH; str[0] != '\0'; str++ ) {
		assert( str[0] >= '0' && str[0] <= '9' );
		hashKey = hashKey * 10 + str[0] - '0';
	}
	return hashKey;
}
