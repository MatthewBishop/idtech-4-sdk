// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETSTATSMANAGER_H__ )
#define __SDNETSTATSMANAGER_H__

//===============================================================
//
//	sdNetStatsManager
//
//===============================================================

struct sdNetStatKeyValue {
	enum statValueType {
		SVT_INT,
		SVT_FLOAT
	};

	union valueData_t {
		int		i;
		float	f;
	};

	const idPoolStr*	key;
	statValueType		type;
	valueData_t			val;
};

typedef idList< sdNetStatKeyValue > sdNetStatKeyValList;

#if !defined( SD_DEMO_BUILD )

class sdNetTask;
struct sdNetClientId;

/*struct sdNetStatLeaderBoardEntry {
	unsigned int
};*/

class sdNetStatsManager {
public:
	virtual					~sdNetStatsManager() {}

	// Write a stats dictionary for a specific client
	virtual bool			WriteDictionary( const sdNetClientId& clientId, const sdNetStatKeyValList& stats ) = 0;

	// Read the stats back that have already been stored for a specific client ( reconnecting clients )
	virtual bool			ReadCachedDictionary( const sdNetClientId& clientId, sdNetStatKeyValList& stats ) = 0;

	//
	// Online functionality
	//

	// Flush pending stats to the master
	virtual sdNetTask*		Flush() = 0;

	// Read a stats dictionary from the master
	virtual sdNetTask*		ReadDictionary( const sdNetClientId& clientId, sdNetStatKeyValList& stats ) = 0;
};

#endif /* !SD_DEMO_BUILD */

#endif /* !__SDNETSTATSMANAGER_H__ */
