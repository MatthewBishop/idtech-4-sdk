// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

/*
===============================================================================

	File System

	No stdio calls should be used by any part of the game, because of all sorts
	of directory and separator char issues. Throughout the game a forward slash
	should be used as a separator. The file system takes care of the conversion
	to an OS specific separator. The file system treats all file and directory
	names as case insensitive.

	The following cvars store paths used by the file system:

	"fs_basepath"		path to local install, read-only
	"fs_savepath"		path to config, save game, etc. files, read & write
	"fs_cdpath"			path to cd, read-only
	"fs_devpath"		path to files created during development, read & write

	The base path for file saving can be set to "fs_savepath" or "fs_devpath".

===============================================================================
*/

static const unsigned	FILE_NOT_FOUND_TIMESTAMP	= 0xFFFFFFFF;
static const int		MAX_PURE_PAKS				= 128;
static const int		MAX_OSPATH					= 256;

// mode parm for OpenFileByMode
typedef enum {
	FS_READ,
	FS_WRITE,
	FS_APPEND,
	FS_APPEND_SYNC
} fsMode_t;

typedef enum {
	PURE_OK,		// we are good to connect as-is
	PURE_RESTART,	// restart required
	PURE_MISSING,	// pak files missing on the client
	PURE_NODLL		// no DLL could be extracted
} fsPureReply_t;

typedef enum {
	DLTYPE_URL,
	DLTYPE_FILE
} dlType_t;

typedef enum {
	DL_WAIT,		// waiting in the list for beginning of the download
	DL_INPROGRESS,	// in progress
	DL_DONE,		// download completed, success
	DL_ABORTING,	// this one can be set during a download, it will force the next progress callback to abort - then will go to DL_FAILED
	DL_FAILED
} dlStatus_t;

typedef enum {
	FILE_EXEC,
	FILE_OPEN
} dlMime_t;

typedef struct urlDownload_s {
	idStr					url;
	int						dltotal;
	int						dlnow;
	int						dlstatus;
	dlStatus_t				status;
} urlDownload_t;

typedef struct fileDownload_s {
	int						position;
	int						length;
	void *					buffer;
} fileDownload_t;

typedef struct backgroundDownload_s {
	struct backgroundDownload_s	*next;	// set by the fileSystem
	dlType_t				opcode;
	idFile *				f;
	fileDownload_t			file;
	urlDownload_t			url;
	volatile bool			completed;
} backgroundDownload_t;

// file list for directory listings
class idFileList {
	friend class idFileSystemLocal;
public:
	const char *			GetBasePath( void ) const { return basePath; }
	int						GetNumFiles( void ) const { return list.Num(); }
	const char *			GetFile( int index ) const { return list[index]; }
	const idStrList &		GetList( void ) const { return list; }

private:
	idStr					basePath;
	idStrList				list;
};

// mod list
class idModList {
	friend class idFileSystemLocal;
public:
	int						GetNumMods( void ) const { return mods.Num(); }
	const char *			GetMod( int index ) const { return mods[index]; }
	const char *			GetDescription( int index ) const { return descriptions[index]; }

private:
	idStrList				mods;
	idStrList				descriptions;
};

class idFileSystem {
public:
	virtual					~idFileSystem() {}
							// Initializes the file system.
	virtual void			Init( void ) = 0;
							// Restarts the file system.
	virtual void			Restart( void ) = 0;
							// Shutdown the file system.
	virtual void			Shutdown( bool reloading ) = 0;
							// Returns true if the file system is initialized.
	virtual bool			IsInitialized( void ) const = 0;
							// Returns true if we are doing an fs_copyfiles.
	virtual bool			PerformingCopyFiles( void ) const = 0;
							// Returns a list of mods found along with descriptions
							// 'mods' contains the directory names to be passed to fs_game
							// 'descriptions' contains a free form string to be used in the UI
	virtual idModList *		ListMods( void ) = 0;
							// Frees the given mod list
	virtual void			FreeModList( idModList *modList ) = 0;
							// Lists files with the given extension in the given directory.
							// Directory should not have either a leading or trailing '/'
							// The returned files will not include any directories or '/' unless fullRelativePath is set.
							// The extension must include a leading dot and may not contain wildcards.
							// If extension is "/", only subdirectories will be returned.
	virtual idFileList *	ListFiles( const char *relativePath, const char *extension, bool sort = false, bool fullRelativePath = false ) = 0;
							// Lists files in the given directory and all subdirectories with the given extension.
							// Directory should not have either a leading or trailing '/'
							// The returned files include a full relative path.
							// The extension must include a leading dot and may not contain wildcards.
	virtual idFileList *	ListFilesTree( const char *relativePath, const char *extension, bool sort = false ) = 0;
							// Frees the given file list.
	virtual void			FreeFileList( idFileList *fileList ) = 0;
							// Converts a relative path to a full OS path.
	virtual const char *	OSPathToRelativePath( const char *OSPath ) = 0;
							// Converts a full OS path to a relative path.
	virtual const char *	RelativePathToOSPath( const char *relativePath, const char *basePath = "fs_devpath" ) = 0;
							// Builds a full OS path from the given components.
	virtual const char *	BuildOSPath( const char *base, const char *game, const char *relativePath ) = 0;
							// Creates the given OS path for as far as it doesn't exist already.
	virtual void			CreateOSPath( const char *OSPath ) = 0;
							// Returns true if a file is in a pak file.
	virtual bool			FileIsInPAK( const char *relativePath ) const = 0;
							// Returns a space separated string containing the checksums of all referenced pak files.
							// will call SetPureServerChecksums internally to restrict itself
	virtual void			UpdatePureServerChecksums( void ) = 0;
							// setup the mapping of OS -> game pak checksum
	virtual bool			UpdateGamePakChecksums( void ) = 0;
							// 0-terminated list of pak checksums
							// if pureChecksums[ 0 ] == 0, all data sources will be allowed
							// otherwise, only pak files that match one of the checksums will be checked for files
							// with the sole exception of .cfg files.
							// the function tries to configure pure mode from the paks already referenced and this new list
							// it returns wether the switch was successfull, and sets the missing checksums
							// the process is verbosive when fs_debug 1
	virtual fsPureReply_t	SetPureServerChecksums( const int pureChecksums[ MAX_PURE_PAKS ], int gamePakChecksum, int missingChecksums[ MAX_PURE_PAKS ], int *missingGamePakChecksum ) = 0;
							// fills a 0-terminated list of pak checksums for a client
							// if OS is -1, give the current game pak checksum. if >= 0, lookup the game pak table (server only)
	virtual void			GetPureServerChecksums( int checksums[ MAX_PURE_PAKS ], int OS, int *gamePakChecksum ) = 0;
							// before doing a restart, force the pure list and the search order
							// if the given checksum list can't be completely processed and set, will error out
	virtual void			SetRestartChecksums( const int pureChecksums[ MAX_PURE_PAKS ], int gamePakChecksum ) = 0;
							// equivalent to calling SetPureServerChecksums with an empty list
	virtual	void			ClearPureChecksums( void ) = 0;
							// get a mask of supported OSes. if not pure, returns -1
	virtual unsigned int	GetOSMask( void ) = 0;
							// Reads a complete file.
							// Returns the length of the file, or -1 on failure.
							// A null buffer will just return the file length without loading.
							// A null timestamp will be ignored.
							// As a quick check for existance. -1 length == not present.
							// A 0 byte will always be appended at the end, so string ops are safe.
							// The buffer should be considered read-only, because it may be cached for other uses.
	virtual int				ReadFile( const char *relativePath, void **buffer, unsigned *timestamp = NULL ) = 0;
							// Frees the memory allocated by ReadFile.
	virtual void			FreeFile( void *buffer ) = 0;
							// Writes a complete file, will create any needed subdirectories.
							// Returns the length of the file, or -1 on failure.
	virtual int				WriteFile( const char *relativePath, const void *buffer, int size, const char *basePath = "fs_savepath" ) = 0;
							// Removes the given file.
	virtual void			RemoveFile( const char *relativePath ) = 0;
							// Opens a file for reading.
	virtual idFile *		OpenFileRead( const char *relativePath, bool allowCopyFiles = true ) = 0;
							// Opens a file for writing, will create any needed subdirectories.
	virtual idFile *		OpenFileWrite( const char *relativePath, const char *basePath = "fs_savepath" ) = 0;
							// Opens a file for writing at the end.
	virtual idFile *		OpenFileAppend( const char *filename, bool sync = false ) = 0;
							// Opens a file for reading, writing, or appending depending on the value of mode.
	virtual idFile *		OpenFileByMode( const char *relativePath, fsMode_t mode ) = 0;
							// Opens a file for reading from a full OS path.
	virtual idFile *		OpenExplicitFileRead( const char *OSPath ) = 0;
							// Opens a file for writing to a full OS path.
	virtual idFile *		OpenExplicitFileWrite( const char *OSPath ) = 0;
							// Closes a file.
	virtual void			CloseFile( idFile *f ) = 0;
							// Returns immediately, performing the read from a background thread.
	virtual void			BackgroundDownload( backgroundDownload_t *bgl ) = 0;
							// resets the bytes read counter
	virtual void			ResetReadCount( void ) = 0;
							// retrieves the current read count
	virtual int				GetReadCount( void ) = 0;
							// adds to the read count
	virtual void			AddToReadCount( int c ) = 0;
							// look for a dynamic module
	virtual void			FindDLL( const char *basename, char dllPath[ MAX_OSPATH ], bool updateChecksum ) = 0;
};

extern idFileSystem *		fileSystem;

#endif /* !__FILESYSTEM_H__ */
