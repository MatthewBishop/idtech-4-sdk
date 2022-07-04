
#ifndef PREDICTED_VALUE_H_
#define PREDICTED_VALUE_H_

#include "Game_local.h"

/*
================================================
A simple class to handle simple predictable values
on multiplayer clients.

The class encapsulates the actual value to be stored
as well as the client frame number on which it is set.

When reading predicted values from a snapshot, the actual
value is only updated if the server has processed the client's
usercmd for the frame in which the client predicted the value.
Got that?
================================================
*/
template< class type_ >
class idPredictedValue {
public:
	explicit	idPredictedValue();
	explicit	idPredictedValue( const type_ & value_ );

	void		Set( const type_ & newValue );

	idPredictedValue< type_ > & operator=( const type_ & value );

	idPredictedValue< type_ > & operator+=( const type_ & toAdd );
	idPredictedValue< type_ > & operator-=( const type_ & toSubtract );

	bool		UpdateFromSnapshot( const type_ & valueFromSnapshot, int clientNumber );

	type_		Get() const { return value; }

private:
	// Noncopyable
	idPredictedValue( const idPredictedValue< type_ > & other );
	idPredictedValue< type_ > & operator=( const idPredictedValue< type_ > & other );

	type_		value;
	int			clientPredictedMilliseconds;	// The time in which the client predicted the value.

	void		UpdatePredictionTime();
};




#endif
