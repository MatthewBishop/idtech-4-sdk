
#ifndef PREDICTED_VALUE_IMPL_H_
#define PREDICTED_VALUE_IMPL_H_

#include "PredictedValue.h"
#include "Player.h"

/*
===============
idPredictedValue::idPredictedValue
===============
*/
template< class type_ >
idPredictedValue< type_ >::idPredictedValue() :
	value(),
	clientPredictedMilliseconds( 0 ) {
}

/*
===============
idPredictedValue::idPredictedValue
===============
*/
template< class type_ >
idPredictedValue< type_ >::idPredictedValue( const type_ & value_ ) :
	value( value_ ),
	clientPredictedMilliseconds( 0 ) {
}

/*
===============
idPredictedValue::UpdatePredictionTime
===============
*/
template< class type_ >
void idPredictedValue< type_ >::UpdatePredictionTime() {
	if ( gameLocal.GetLocalPlayer() != NULL ) {
		clientPredictedMilliseconds = gameLocal.GetLocalPlayer()->usercmd.clientGameMilliseconds;
	}
}

/*
===============
idPredictedValue::Set
===============
*/
template< class type_ >
void idPredictedValue< type_ >::Set( const type_ & newValue ) {
	value = newValue;
	UpdatePredictionTime();
}

/*
===============
idPredictedValue::operator=
===============
*/
template< class type_ >
idPredictedValue< type_ > & idPredictedValue< type_ >::operator=( const type_ & newValue ) {
	Set( newValue );
	return *this;
}

/*
===============
idPredictedValue::operator+=
===============
*/
template< class type_ >
idPredictedValue< type_ > & idPredictedValue< type_ >::operator+=( const type_ & toAdd ) {
	Set( value + toAdd );
	return *this;
}

/*
===============
idPredictedValue::operator-=
===============
*/
template< class type_ >
idPredictedValue< type_ > & idPredictedValue< type_ >::operator-=( const type_ & toSubtract ) {
	Set( value - toSubtract );
	return *this;
}

/*
===============
idPredictedValue::UpdateFromSnapshot

Always updates the value for remote clients.

Only updates the actual value if the snapshot usercmd frame is newer than the one in which
the client predicted this value.

Returns true if the value was set, false if not.
===============
*/
template< class type_ >
bool idPredictedValue< type_ >::UpdateFromSnapshot( const type_ & valueFromSnapshot, int clientNumber ) {
	if ( clientNumber != gameLocal.GetLocalClientNum() ) {
		value = valueFromSnapshot;
		return true;
	}
		
	if ( gameLocal.GetLastClientUsercmdMilliseconds( clientNumber ) >= clientPredictedMilliseconds ) {
		value = valueFromSnapshot;
		return true;
	}

	return false;
}

/*
===============
operator==

Overload for idPredictedValue.
We only care if the values are equal, not the frame number.
===============
*/
template< class firstType_, class secondType_ >
bool operator==( const idPredictedValue< firstType_ > & lhs, const idPredictedValue< secondType_ > & rhs ) {
	return lhs.Get() == rhs.Get();
}

/*
===============
operator!=

Overload for idPredictedValue.
We only care if the values are equal, not the frame number.
===============
*/
template< class firstType_, class secondType_ >
bool operator!=( const idPredictedValue< firstType_ > & lhs, const idPredictedValue< secondType_ > & rhs ) {
	return lhs.Get() != rhs.Get();
}

/*
===============
operator==

Overload for idPredictedValue.
We only care if the values are equal, not the frame number.
===============
*/
template< class firstType_, class secondType_ >
bool operator==( const idPredictedValue< firstType_ > & lhs, const secondType_ & rhs ) {
	return lhs.Get() == rhs;
}

/*
===============
operator==

Overload for idPredictedValue.
We only care if the values are equal, not the frame number.
===============
*/
template< class firstType_, class secondType_ >
bool operator==( const firstType_ lhs, const idPredictedValue< secondType_ > & rhs ) {
	return lhs == rhs.Get();
}

/*
===============
operator!=

Overload for idPredictedValue.
We only care if the values are equal, not the frame number.
===============
*/
template< class firstType_, class secondType_ >
bool operator!=( const idPredictedValue< firstType_ > & lhs, const secondType_ & rhs ) {
	return lhs.Get() != rhs;
}

/*
===============
operator!=

Overload for idPredictedValue.
We only care if the values are equal, not the frame number.
===============
*/
template< class firstType_, class secondType_ >
bool operator!=( const firstType_ lhs, const idPredictedValue< secondType_ > & rhs ) {
	return lhs != rhs.Get();
}




#endif
