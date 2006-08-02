/*=============================================================================

	Main core header

=============================================================================*/


#ifndef __CORE_H__
#define __CORE_H__


#ifndef CORE_API
#	define CORE_API PACKAGE_IMPORT
#endif


// static/dynamic build macros
#if !STATIC_BUILD
#	define PACKAGE_IMPORT	DLL_IMPORT
#	define PACKAGE_EXPORT	DLL_EXPORT
#else
#	define PACKAGE_IMPORT
#	define PACKAGE_EXPORT
#endif


// C standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <math.h>

// Build options
#include "Build.h"

// Compiler-specific code
#if _MSC_VER
#	include "CoreVisualC.h"
#elif __GNUC__
#	include "CoreGnuC.h"
#else
#	error Unknown compiler
#endif

// Platform-specific code
#if _WIN32
#	include "CoreWin32.h"
#else
#	include "CoreUnix.h"
#endif


// guard/unguard/unguardf[Slow] macros
#if !DO_GUARD
#	undef guard
#	undef unguard
#	undef unguardf
#	define guard(func)
#	define unguard
#	define unguardf(msg)
#endif

#if DO_GUARD_SLOW
#	define guardSlow(func)		guard(func)
#	define unguardSlow			unguard
#	define unguardfSlow(msg)	unguardf(msg)
#else
#	define guardSlow(func)		{
#	define unguardSlow			}
#	define unguardfSlow(msg)	}
#endif


//?? generalize these macros, separate header; GCC only?
#if 0
//?? debugging -- useful, when guard/unguard does not works
#undef guard
#undef unguard
#undef unguardf
void DebugPrintf (const char *str, ...) PRINTF(1,2);
#define guard(func)				DebugPrintf("> %s : %s\n", __func__, #func);
#define unguard					DebugPrintf("< %s\n", __func__);
#define unguardf(msg)			DebugPrintf("< %s : %s\n", __func__, va msg);
#endif

#if 0
extern float *GGuardVar;
extern float GGuardValue;
#undef guard
#undef unguard
#undef unguardf
#define guard(func)				{if (GGuardVar && (GGuardValue!=*GGuardVar)) \
								{ GGuardValue=*GGuardVar; appPrintf(">%s: %g (r:%X)\n",__func__,GGuardValue,GET_RETADDR(nn)); }}
#define unguard					{if (GGuardVar && (GGuardValue!=*GGuardVar)) \
								{ GGuardValue=*GGuardVar; appPrintf("<%s: %g (r:%X)\n",__func__,GGuardValue,GET_RETADDR(nn)); }}
#define unguardf(msg)			unguard
#endif



// Static assertion: some assertions are compile-time, but preprocessor do not allow us
// using in "#if (condition)" something, other than #define'd consts. Use staticAssert()
// for this purpose.
#if 0
#define staticAssert(expr,name)		\
	if (!(expr)) appError ("Assertion: %s (%s)", #expr, __FILE__);
#endif

// helper declararion
template<int> struct CompileTimeError;
template<>    struct CompileTimeError<true> {};

#define staticAssert(expr,name)			\
	{									\
		CompileTimeError<(expr) != 0> assert_##name; \
		(void)assert_##name;			\
	}



#if MAX_DEBUG
#define assert(expr)			if (!(expr)) appError ("Assertion: %s (%s)", #expr, __FILE__);
#else
#define assert(expr)
#endif

// profiling
inline void clock (unsigned &time)		{ time -= appCycles(); }
inline void unclock (unsigned &time)	{ time += appCycles(); }
inline void clock (int64 &time)			{ time -= appCycles64(); }
inline void unclock (int64 &time)		{ time += appCycles64(); }


#if STATS
#	define STAT(x)				x
#else
#	define STAT(x)
#endif


/*-----------------------------------------------------------------------------
	Variables for current package
-----------------------------------------------------------------------------*/

#ifndef PACKAGE
#define PACKAGE					Main
#endif

#define MAX_PACKAGES			32
#define MAX_PKG_NAME			32

// package variables
#if STATIC_BUILD
#	undef IMPLEMENT_PACKAGE
#	define IMPLEMENT_PACKAGE(version,build,date)	\
	namespace PACKAGE {								\
		const char PkgName[]    = STR(PACKAGE);		\
		const char PkgVersion[] = STR(PACKAGE) " version " STR(version) " build " STR(build) " (" date ")"; \
	}

namespace PACKAGE {
	extern const char PkgName[];
	extern const char PkgVersion[];
}

#define GPackage				PACKAGE.PkgName
#define GPkgVersion				PACKAGE.PkgVersion

#else // STATIC_BUILD

extern const char GPackage[];
extern const char PkgVersion[];

#endif // STATIC_BUILD


/*----------------------------------------------------------------------------
	Forward declarations
----------------------------------------------------------------------------*/

class CMemoryChain;
struct CVec3;


//----- miscellaneous ?

#define BIG_NUMBER				0x1000000


#include "Macro.h"
#include "Strings.h"


/*-----------------------------------------------------------------------------
	Text output system
-----------------------------------------------------------------------------*/

// Color codes

#define C_BLACK			0
#define C_RED			1
#define C_GREEN			2
#define C_YELLOW		3
#define C_BLUE			4
#define C_MAGENTA		5
#define C_CYAN			6
#define C_WHITE			7

#define COLOR_ESCAPE	'^'		// may be used for quick location of color-processing code

#define S_BLACK			"^0"
#define S_RED			"^1"
#define S_GREEN			"^2"
#define S_YELLOW		"^3"
#define S_BLUE			"^4"
#define S_MAGENTA		"^5"
#define S_CYAN			"^6"
#define S_WHITE			"^7"


CORE_API void appPrintf (const char *fmt, ...) PRINTF(1,2);
CORE_API void appWPrintf (const char *fmt, ...) PRINTF(1,2);


// Output device
class CORE_API COutputDevice
{
public:
	bool	NoColors;			// when true, color codes will be removed from output
	bool	FlushEveryTime;		// when true, Flush() will be called after every Write()
	COutputDevice ()
	:	NoColors(false)
	,	FlushEveryTime(false)
	{}
	virtual ~COutputDevice ()
	{}
	virtual void Write (const char *str) = 0;
	virtual void Flush ()
	{ /* empty */ }
	virtual void Close ()		// may be used instead of destructor
	{ /* empty */ }
	void Printf (const char *fmt, ...) PRINTF(2,3);
	void Register ();
	void Unregister ();
};

// macros for hook/unhook output
#define HookOutput(device)				\
	COutputDevice *_OldHook = GLogHook;	\
	GLogHook = device;

#define HookOutputNull					\
	HookOutput(GNull)

#define UnhookOutput					\
	GLogHook = _OldHook;


/*-----------------------------------------------------------------------------
	Process control
-----------------------------------------------------------------------------*/

// Initialize core
CORE_API void appInit (/*?? const char *_cmdLine, COutputDevice *_log, CErrorHandler *_err */);
CORE_API void appExit ();

// System information
CORE_API const char *appPackage ();


/*-----------------------------------------------------------------------------
	Timing functions
-----------------------------------------------------------------------------*/

CORE_API unsigned appMilliseconds ();
CORE_API const char *appTimestamp ();
CORE_API void appSleep (unsigned msec);


/*-----------------------------------------------------------------------------
	Byte-order functions
-----------------------------------------------------------------------------*/

#if LITTLE_ENDIAN

#define LittleShort(n)	((short)n)
#define LittleLong(n)	((long)n)
#define LittleFloat(n)	((float)n)
#define LTL(n)

#else

short	LittleShort (short l);
int		LittleLong (int l);
float	LittleFloat (float l);

inline void LTL(short &v)
{
	v = LittleShort (v);
}
inline void LTL(unsigned short &v)
{
	v = LittleShort (v);
}
inline void LTL(int &v)
{
	v = LittleLong (v);
}
inline void LTL(unsigned &v)
{
	v = LittleLong (v);
}
inline void LTL(float &v)
{
	v = LittleFloat (v);
}
inline void LTL(CVec3 &v)
{
	for (int i = 0; i < 3; i++)
		v[i] = LittleFloat (v[i]);
}

#endif


/*-----------------------------------------------------------------------------
	Core includes
-----------------------------------------------------------------------------*/

#include "ErrorMgr.h"
#include "DbgSymbols.h"						// MAY have some macros, so go as first as possible
#include "MemoryMgr.h"
//!! #include "Cvar.h"
#include "Commands.h"
#include "FileSystem.h"
#include "ScriptParser.h"
#include "TextContainer.h"

#include "MathMisc.h"
#include "Math3D.h"


/*-----------------------------------------------------------------------------
	Global variables
-----------------------------------------------------------------------------*/

// output system
CORE_API extern COutputDevice	*GLogHook;				// hook appPrintf() output
CORE_API extern COutputDevice	*GLog;					// output via appPrintf()
CORE_API extern COutputDevice	*GNull;					// do not output
CORE_API extern int				GScreenWidth;			// number of characters per output line

// system information
CORE_API extern char			GMachineOS[];
CORE_API extern char			GMachineCPU[];
//??CORE_API extern TString<512>	GVersion;

// process control
CORE_API extern bool			GIsRequestingExit;


#endif // __CORE_H__
