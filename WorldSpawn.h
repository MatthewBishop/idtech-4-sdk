

#ifndef __GAME_WORLDSPAWN_H__
#define __GAME_WORLDSPAWN_H__

/*
===============================================================================

  World entity.

===============================================================================
*/

class idWorldspawn : public idEntity {
public:
	CLASS_PROTOTYPE( idWorldspawn );

					~idWorldspawn();

	void			Spawn();

	void			Save( idSaveGame *savefile );
	void			Restore( idRestoreGame *savefile );

private:
	void			Event_Remove();
};

#endif /* !__GAME_WORLDSPAWN_H__ */
