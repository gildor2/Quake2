/*=============================================================================

	Core definitions for Win32/VisualC++ platform

=============================================================================*/

// next line will create EBP frames for ALL functions, so, when application
// crashed, we will get complete call stack trace in error logfile
//?? make this as #define option? may be specified in VC command line "/Oy-"
//?? WARNING: call stack will not be displayed, when crashed in some library,
//??  compiled without this option + when error generated with appError()
//#pragma optimize("y", off)

/*-----------------------------------------------------------------------------
	Platform-specific type defines
-----------------------------------------------------------------------------*/

typedef unsigned char	byte;		//?? change name to BYTE etc
//??typedef unsigned short	word;
//??typedef unsigned int	dword;
//??typedef unsigned __int64 qword;

typedef unsigned int	address_t;


// if windows.h is not included ...
#ifndef _WINDOWS_
#	define HANDLE		void*
#	define HINSTANCE	void*
#endif


#define LITTLE_ENDIAN

// this functions are smaller. when in intrinsic form (and, of course, faster):
#pragma intrinsic(memcpy, memset, memcmp, abs, fabs)
// allow nested inline expansions
#pragma inline_depth(8)

/*-----------------------------------------------------------------------------
	Miscellaneous
-----------------------------------------------------------------------------*/

#define DLL_IMPORT	__declspec(dllimport)
#define DLL_EXPORT	__declspec(dllexport)
#define NORETURN	__declspec(noreturn)

#define	GET_RETADDR(firstarg)	(* ( ((unsigned*)&firstarg) -1 ) )

#define vsnprintf _vsnprintf


// Package implementation
#define IMPLEMENT_PACKAGE(version,build,date)		\
	extern const char GPackage[]   = STR(PACKAGE);	\
	extern const char PkgVersion[] = STR(PACKAGE) " version " STR(version) " build " STR(build) " (" date ")";\
	HINSTANCE hInstance;							\
	int __stdcall DllMain (HINSTANCE hInst, int Reason, void *Reserved)	\
	{ hInstance = hInst; return 1; }


// disable some compiler warnings
#pragma warning(disable : 4018)			// signed/unsigned mismatch
#pragma warning(disable : 4291)			// no matched operator delete found
#pragma warning(disable : 4275)			// non dll-interface class used as base for dll-interface class
#pragma warning(disable : 4305)			// truncation from 'const double' to 'const float'
#pragma warning(disable : 4244)			// conversion from 'int'/'double' to 'float'
#pragma warning(disable : 4509)			// nonstandard extension used: function uses SEH and object has destructor


const char *appGetSystemErrorMessage (unsigned code);
extern bool GIsWinNT, GIsWin2K;
extern HINSTANCE hInstance;


/*-----------------------------------------------------------------------------
	Some math functions
-----------------------------------------------------------------------------*/

inline int appRound (float f)
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

inline int appFloor (float f)
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

inline int appCeil (float f)
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
#pragma warning(disable : 4035)		// "no return value"

inline unsigned appCycles ()
{
//	__asm push edx
	__asm rdtsc
//	__asm pop edx
}

inline __int64 appCycles64 ()
{
	__asm rdtsc
}
#pragma warning(pop)


/*-----------------------------------------------------------------------------
	guard/unguard macros
-----------------------------------------------------------------------------*/

#if !WIN32_USE_SEH

#define guard(func)						\
	{									\
		static const char __FUNC__[] = #func; \
		try {

#define unguard							\
		} catch (...) {					\
			appUnwindThrow (__FUNC__);	\
		}								\
	}

#define unguardf(msg)					\
		} catch (...) {					\
			appUnwindPrefix (__FUNC__);	\
			appUnwindThrow msg;			\
		}								\
	}

#define TRY		try
#define CATCH	catch (...)

#else // WIN32_USE_SEH

#if 0
#include <excpt.h>
CORE_API int win32ExceptFilter (struct _EXCEPTION_POINTERS *info);
#define EXCEPT_FILTER	win32ExceptFilter(GetExceptionInformation())
#else
// this will allow to produce slightly smaller code, but may not work under non-VC6 compiler
CORE_API int win32ExceptFilter2 ();
#define EXCEPT_FILTER	win32ExceptFilter2()
#endif


#define guard(func)						\
	{									\
		static const char __FUNC__[] = #func; \
		__try {

#define unguard							\
		} __except (EXCEPT_FILTER) {	\
			appUnwindThrow (__FUNC__);	\
		}								\
	}

#define unguardf(msg)					\
		} __except (EXCEPT_FILTER) {	\
			appUnwindPrefix (__FUNC__);	\
			appUnwindThrow msg;			\
		}								\
	}

#define TRY		__try
#define CATCH	__except (EXCEPT_FILTER)

#endif // WIN32_USE_SEH
