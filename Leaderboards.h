

#ifndef __LEADERBOARDS_LOCAL_H__
#define __LEADERBOARDS_LOCAL_H__


struct leaderboardStats_t {
	int		frags;
	int		wins;
	int		teamfrags;
	int		deaths;
};

struct columnGameMode_t {

	columnDef_t *	columnDef;					// The Column definition for the game mode.
	int				numColumns;					
	rankOrder_t		rankOrder;					// rank ordering of the game mode. (  RANK_GREATEST_FIRST, RANK_LEAST_FIRST )
	bool			supportsAttachments;		
	bool			checkAgainstCurrent;
	const char *	abrevName;					// Leaderboard Game Mode Abrev.
};

/*
================================================================================================

	Leaderboards

================================================================================================
*/



// creates and stores all the leaderboards inside the internal map ( see Sys_FindLeaderboardDef on retrieving definition )
void			LeaderboardLocal_Init();

// Destroys any leaderboard definitions allocated by LeaderboardLocal_Init() 
void			LeaderboardLocal_Shutdown();

// Gets a leaderboard ID with map index and game type.
int				LeaderboardLocal_GetID( int mapIndex, int gametype );

// Do it all function. Will create the column_t with the correct stats from the game type, and upload it to the leaderboard system.
void			LeaderboardLocal_Upload(  lobbyUserID_t lobbyUserID, int gameType, leaderboardStats_t & stats );

extern const columnGameMode_t gameMode_columnDefs[];

#endif // __LEADERBOARDS_LOCAL_H__
