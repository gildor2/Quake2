/*=============================================================================
	Engine build options
=============================================================================*/

#define MEM_DEBUG		1			// comment when unneeded
#define MAX_DEBUG		1
//#define NO_DEBUG		1
#define FPU_EXCEPTIONS	1

//#define DO_GUARD		0


/*-----------------------------------------------------------------------------
	Debugging defaults (MAX_DEBUG/NO_DEBUG)
-----------------------------------------------------------------------------*/

#ifndef MAX_DEBUG
#define MAX_DEBUG		0
#endif

#ifndef NO_DEBUG
#define NO_DEBUG		0
#endif


#if MAX_DEBUG && NO_DEBUG
#error MAX_DEBUG=1 && NO_DEBUG=1
#endif

#if MAX_DEBUG || NO_DEBUG

#if MAX_DEBUG
#define DEBUG_MODE		1
#else
#define DEBUG_MODE		0
#endif

#ifndef DO_GUARD
#define DO_GUARD		DEBUG_MODE
#endif

#ifndef DO_GUARD_SLOW
#define DO_GUARD_SLOW	DEBUG_MODE
#endif

#ifndef DBG_SYMBOLS
#define DBG_SYMBOLS		DEBUG_MODE
#endif

#ifndef MEM_DEBUG
#define MEM_DEBUG		DEBUG_MODE
#endif

#ifndef MEM_STATS
#define MEM_STATS		DEBUG_MODE
#endif

#ifndef STATS
#define STATS			DEBUG_MODE
#endif

#ifndef FPU_EXCEPTIONS
#define FPU_EXCEPTIONS	DEBUG_MODE
#endif

#endif // MAX_DEBUG || NO_DEBUG


/*-----------------------------------------------------------------------------
	Defaults (put this section to a separate header ?)
-----------------------------------------------------------------------------*/

//?? should be always 1; remove?
#ifndef WIN32_USE_SEH
#define WIN32_USE_SEH	1			// when 0, will be compiled with C++ exception system, or when 1 - VC++ exception system
#endif


// debugging

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

#ifndef STATS
#define STATS			1
#endif

#ifndef FPU_EXCEPTIONS
#define FPU_EXCEPTIONS	0
#endif
