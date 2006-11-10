/*=============================================================================

	Core definitions for VisualC++ compiler

=============================================================================*/

#if MAX_DEBUG
// Create EBP frames for ALL functions, so, when application crashed, we will
//   get complete call stack trace in error logfile
// NOTE: same effect with VC command line "/Oy-"
// WARNING: call stack will not be displayed, when crashed in some library,
//   compiled without this option + when error generated with appError()
#pragma optimize("y", off)
#endif


/*-----------------------------------------------------------------------------
	Type defines
-----------------------------------------------------------------------------*/

#define LITTLE_ENDIAN		1

typedef unsigned char		byte;		//?? change name to BYTE etc
//??typedef unsigned short	word;
//??typedef unsigned int	dword;
typedef __int64				int64;		//?? INT64

typedef unsigned int		address_t;


/*-----------------------------------------------------------------------------
	Miscellaneous
-----------------------------------------------------------------------------*/

// this functions are smaller, when in intrinsic form (and, of course, faster):
#pragma intrinsic(memcpy, memset, memcmp, abs, fabs)
// allow nested inline expansions
#pragma inline_depth(8)

#define DLL_IMPORT			__declspec(dllimport)
#define DLL_EXPORT			__declspec(dllexport)
#define NORETURN			__declspec(noreturn)
#define PRINTF(n,m)						// no way ...

#if _MSC_VER >= 1200
#	define FORCEINLINE		__forceinline
#else
#	define FORCEINLINE		inline
#endif
#if _MSC_VER >= 1300
#	define NOINLINE			__declspec(noinline)
#else
#	define NOINLINE						// no way ...
#endif

#define GCC_PACK						// VC uses #pragma pack()


#define	GET_RETADDR(firstarg)	(* ( ((unsigned*)&firstarg) -1 ) )

#define vsnprintf _vsnprintf


// disable some compiler warnings
#pragma warning(disable : 4018)			// signed/unsigned mismatch
#pragma warning(disable : 4244)			// conversion from 'int'/'double' to 'float'
#pragma warning(disable : 4251)			// class 'Name' needs to have dll-interface to be used by clients of class 'Class'
#pragma warning(disable : 4275)			// non dll-interface class used as base for dll-interface class
#pragma warning(disable : 4291)			// no matched operator delete found
#pragma warning(disable : 4305)			// truncation from 'const double' to 'const float'
#pragma warning(disable : 4509)			// nonstandard extension used: function uses SEH and object has destructor


/*-----------------------------------------------------------------------------
	Some math functions
-----------------------------------------------------------------------------*/

inline int appRound(float f)
{
	int		i;

	__asm {
		fld		[f]
		fistp	[i]
	}
	return i;
}

// NOTE: when used 0.5 in floor/ceil functions, integral values
// will be rounded incorrectly (floor(1)=0,floor(2)=2;floor(3)=2 etc.)

inline int appFloor(float f)
{
	int		i;
	static const float h = 0.4999999;	// 0.5

	__asm {
		fld		[f]
		fsub	[h]
		fistp	[i]
	}
	return i;
}

inline int appCeil(float f)
{
	int		i;
	static const float h = 0.4999999;	// 0.5

	__asm {
		fld		[f]
		fadd	[h]
		fistp	[i]
	}
	return i;
}


/*-----------------------------------------------------------------------------
	Pentium RDTSC timing
-----------------------------------------------------------------------------*/

#pragma warning(push)
#pragma warning(disable : 4035)			// "no return value"

inline unsigned appCycles()
{
	__asm rdtsc
}

inline int64 appCycles64()
{
	__asm rdtsc
}
#pragma warning(pop)


extern CORE_API double GMSecondsPerCycle;

inline float appCyclesToMsecf(unsigned timeDelta)
{
	double v = timeDelta;
	return v * GMSecondsPerCycle;
}

inline float appCyclesToMsecf(int64 &timeDelta)
{
	double v = timeDelta;
	return v * GMSecondsPerCycle;
}


/*-----------------------------------------------------------------------------
	guard/unguard macros
-----------------------------------------------------------------------------*/

/* NOTES:
 *	- for error throwing, we use "throw 1" operator; "1" is required for WIN32_USE_SEH=0 compilation
 *	  for appError()/appNonFatalError() (otherwise, unhandled GPF will be generated)
 *	- for win32+VC we should use WIN32_USE_SEH=1; ==0 works too, but for VisualC exception system only
 *	  (GnuC/mingw32 have completely different exception system: OS exceptions are not converted to C++
 *	  exceptions)
 *	- major difference between SEH/C++ exception tracking: for SEH, when we entering CATCH{} block,
 *	  GIsFatalError=true and exception context already dumped; for C++ system, GIsFatalError=false and
 *	  nothing dumped ...
 */


#if !WIN32_USE_SEH
// really, this should not be used ...

#define guard(func)						\
	{									\
		static const char __FUNC__[] = #func; \
		try {

#define unguard							\
		} catch (...) {					\
			appUnwindThrow(__FUNC__);	\
		}								\
	}

#define unguardf(msg)					\
		} catch (...) {					\
			appUnwindPrefix(__FUNC__);	\
			appUnwindThrow msg;			\
		}								\
	}

#define TRY_CRASH		try
#define CATCH_CRASH		catch (...)
#define END_CATCH
#define TRY				try
#define CATCH			catch (...)
#define	THROW_AGAIN		throw
#define THROW			throw 1

#else // WIN32_USE_SEH

#if 0
#include <excpt.h>
CORE_API unsigned WINAPI win32ExceptFilter(struct _EXCEPTION_POINTERS *info);
#define EXCEPT_FILTER	win32ExceptFilter(GetExceptionInformation())
#else
// this will allow to produce slightly smaller code, but may not work under non-VC6 compiler
CORE_API unsigned win32ExceptFilter2();
#define EXCEPT_FILTER	win32ExceptFilter2()
#endif


#define guard(func)						\
	{									\
		static const char __FUNC__[] = #func; \
		__try {

#define unguard							\
		} __except (EXCEPT_FILTER) {	\
			appUnwindThrow(__FUNC__);	\
		}								\
	}

#define unguardf(msg)					\
		} __except (EXCEPT_FILTER) {	\
			appUnwindPrefix(__FUNC__);	\
			appUnwindThrow msg;			\
		}								\
	}

#define TRY_CRASH		__try
#define CATCH_CRASH		__except(EXCEPT_FILTER)
#define END_CATCH

#define TRY				__try
#define CATCH			__except(1)			// 1==EXCEPTION_EXECUTE_HANDLER

#define THROW_AGAIN		throw
#define THROW			throw 1

#endif // WIN32_USE_SEH
