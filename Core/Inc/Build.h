// Build options

#define MEM_DEBUG		1			//?? comment when unneeded

#define	APP_VERSION				0.01

/*-----------------------------------------------------------------------------
	these consts should not be inside engine (q2 launcher)
-----------------------------------------------------------------------------*/

#define APPNAME						"Quake2"					//?? unneeded
#define	BASEDIRNAME					"baseq2"
#define CONFIGNAME					"config.cfg"				//?? unneeded


// if CD_PATH is not defined, CD checking and adding its paths will not be performed
#define CD_PATH						"install/data"
#define CD_CHECK					"install/data/quake2.exe"	// file used for CD validation


#define SAVEGAME_DIRECTORY			"save"
#define SAVEGAME_SERVER_EXTENSION	"sv2"
#define SAVEGAME_GAME_EXTENSION		"sav"
#define SAVEGAME_VARS_EXTENSION		"ssv"


//?? should not be here
#define NEW_PROTOCOL_ID				"gildor"


/*-----------------------------------------------------------------------------
	Defaults (put this section to a separate header ?)
-----------------------------------------------------------------------------*/

#ifndef WIN32_USE_SEH
#define WIN32_USE_SEH	1			// when 0, will be compiled with C++ exception system, or when 1 - VC++ exception system
#endif

#ifndef DO_GUARD
#define DO_GUARD		1
#endif

#ifndef DO_GUARD_SLOW
#define DO_GUARD_SLOW	0
#endif

#ifndef DBG_SYMBOLS
#define DBG_SYMBOLS		1
#endif

#ifndef MEM_DEBUG
#define MEM_DEBUG		0
#endif

#ifndef MEM_STATS
#define MEM_STATS		0
#endif

//?? move to core.h
#define MAX_PACKAGES	32
#define MAX_PKG_NAME	32
