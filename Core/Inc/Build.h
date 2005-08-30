// Build options

#define MEM_DEBUG		1			//?? comment when unneeded

#define	APP_VERSION		0.01		//?? unused

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
